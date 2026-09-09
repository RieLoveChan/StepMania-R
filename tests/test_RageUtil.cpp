// Characterization tests for src/RageUtil string/path helpers.
//
// These pin the engine's CURRENT behaviour, bug-for-bug — including the
// quirks (see GetExtension with a slash after the dot, and Capitalize
// only touching the first codepoint). If a refactor changes an outcome here,
// that is a signal to stop and decide whether the change is intended, not
// a licence to edit the expectation. See
// DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "RageUtil.h"

#include "catch_amalgamated.hpp"

TEST_CASE("Trim removes leading and trailing whitespace by default", "[RageUtil][string]")
{
	RString s = "  \t hello \r\n";
	Trim(s);
	CHECK(s == "hello");

	RString all_ws = " \t\r\n";
	Trim(all_ws);
	CHECK(all_ws.empty());

	RString none = "abc";
	Trim(none);
	CHECK(none == "abc");

	SECTION("custom trim set")
	{
		RString x = "xxabcxx";
		Trim(x, "x");
		CHECK(x == "abc");
	}
}

TEST_CASE("TrimLeft / TrimRight trim only one side", "[RageUtil][string]")
{
	RString l = "  ab  ";
	TrimLeft(l);
	CHECK(l == "ab  ");

	RString r = "  ab  ";
	TrimRight(r);
	CHECK(r == "  ab");
}

TEST_CASE("GetExtension returns the text after the last dot", "[RageUtil][path]")
{
	CHECK(GetExtension("song.ssc") == "ssc");
	CHECK(GetExtension("archive.tar.gz") == "gz");

	// No dot at all -> empty.
	CHECK(GetExtension("README").empty());

	// Quirk pinned on purpose: a '/' anywhere after the last dot makes it
	// bail out with an empty string ("path/dir.ext/fn").
	CHECK(GetExtension("dir.d/file").empty());
}

TEST_CASE("GetFileNameWithoutExtension strips directory and extension", "[RageUtil][path]")
{
	CHECK(GetFileNameWithoutExtension("Songs/Group/song.ssc") == "song");
	CHECK(GetFileNameWithoutExtension("noext") == "noext");
	CHECK(GetFileNameWithoutExtension("a.b.c") == "a.b");
}

TEST_CASE("SetExtension swaps the extension, keeping the directory", "[RageUtil][path]")
{
	CHECK(SetExtension("Songs/Group/song.ssc", "sm") == "Songs/Group/song.sm");
	CHECK(SetExtension("song.ssc", "") == "song");
	CHECK(SetExtension("noext", "dat") == "noext.dat");
}

TEST_CASE("Basename returns the last path component, ignoring trailing slashes", "[RageUtil][path]")
{
	CHECK(Basename("/foo/bar/baz") == "baz");
	CHECK(Basename("/foo/bar/") == "bar");
	CHECK(Basename("noslash") == "noslash");
	CHECK(Basename("///").empty());

	// Backslashes count as separators too.
	CHECK(Basename("foo\\bar") == "bar");
}

TEST_CASE("BinaryToHex renders lowercase, zero-padded, two chars per byte", "[RageUtil][hex]")
{
	const unsigned char bytes[] = { 0x00, 0x0f, 0xa5, 0xff };
	CHECK(BinaryToHex(bytes, sizeof(bytes)) == "000fa5ff");
	CHECK(BinaryToHex(RString("AB")) == "4142");
	CHECK(BinaryToHex(nullptr, 0).empty());
}

TEST_CASE("ssprintf formats like printf into an RString", "[RageUtil][string]")
{
	CHECK(ssprintf("%02x", 255) == "ff");
	CHECK(ssprintf("%d-%d", 1, 2) == "1-2");
	CHECK(ssprintf("%s!", "hi") == "hi!");
}

// StringConversion::FromString<T> / ToString<T> back the Preference<T>
// serialisation (Preferences.ini, --flags). Pin the quirks: trailing
// junk tolerated, bool via StringToInt (not "true"/"false"), float
// rejects non-finite, ToString<bool> emits "0"/"1".
TEST_CASE("StringConversion::FromString<int> takes the leading integer, else 0/false", "[RageUtil][conv]")
{
	int v = -99;
	CHECK(StringConversion::FromString("42", v));      CHECK(v == 42);
	CHECK(StringConversion::FromString("-7", v));      CHECK(v == -7);
	CHECK(StringConversion::FromString("12abc", v));   CHECK(v == 12);   // trailing junk ignored
	CHECK_FALSE(StringConversion::FromString("abc", v)); CHECK(v == 0);  // failure zeroes out
	CHECK_FALSE(StringConversion::FromString("", v));  CHECK(v == 0);
}

TEST_CASE("StringConversion::FromString<float> parses a leading number and rejects non-finite", "[RageUtil][conv]")
{
	float v = -99;
	CHECK(StringConversion::FromString("1.5", v));     CHECK(v == Catch::Approx(1.5f));
	CHECK(StringConversion::FromString("2.5x", v));    CHECK(v == Catch::Approx(2.5f)); // trailing junk ignored
	CHECK_FALSE(StringConversion::FromString("nope", v)); CHECK(v == 0.0f);
	CHECK_FALSE(StringConversion::FromString("", v));  CHECK(v == 0.0f);
	// A value that parses but is not finite is rejected (out zeroed).
	CHECK_FALSE(StringConversion::FromString("1e999", v)); CHECK(v == 0.0f);
}

TEST_CASE("StringConversion::FromString<bool> is StringToInt() != 0, empty -> false-return", "[RageUtil][conv]")
{
	bool v = true;
	CHECK(StringConversion::FromString("0", v));  CHECK(v == false);
	CHECK(StringConversion::FromString("1", v));  CHECK(v == true);
	CHECK(StringConversion::FromString("5", v));  CHECK(v == true);
	CHECK(StringConversion::FromString("-3", v)); CHECK(v == true);
	// "true" is not recognised: StringToInt("true") fails the stoi parse
	// and returns the exception value (0) -> false. (The stoi catch path
	// is LOG-guarded, so this no longer needs a live engine.)
	CHECK(StringConversion::FromString("true", v)); CHECK(v == false);
	// Empty string: the call returns false and leaves v alone.
	v = true;
	CHECK_FALSE(StringConversion::FromString("", v));
	CHECK(v == true);
}

TEST_CASE("StringConversion::ToString<T> and the FromString round-trip", "[RageUtil][conv]")
{
	CHECK(StringConversion::ToString(42) == "42");
	CHECK(StringConversion::ToString(-7) == "-7");
	CHECK(StringConversion::ToString(true) == "1");   // "%i", not "true"
	CHECK(StringConversion::ToString(false) == "0");
	CHECK(StringConversion::ToString(1.5f) == "1.500000"); // "%f" -> 6 decimals

	for (int i : { 0, 1, -1, 123456, -987654 })
	{
		int back = 0;
		REQUIRE(StringConversion::FromString(StringConversion::ToString(i), back));
		CHECK(back == i);
	}
	for (bool b : { true, false })
	{
		bool back = !b;
		REQUIRE(StringConversion::FromString(StringConversion::ToString(b), back));
		CHECK(back == b);
	}
}

// split() / join() -- the workhorses under every simfile parse, command
// string and config line. Pins: an empty source yields an empty vector
// even with bIgnoreEmpty=false; bIgnoreEmpty controls whether empty
// fields survive; join is the exact inverse when empties are kept.
TEST_CASE("split on a single-char delimiter, with and without bIgnoreEmpty", "[RageUtil][split]")
{
	std::vector<RString> v;

	split("a,b,c", ",", v, true);
	REQUIRE(v.size() == 3);
	CHECK(v[0] == "a"); CHECK(v[1] == "b"); CHECK(v[2] == "c");

	v.clear();
	split("a,,c", ",", v, /*bIgnoreEmpty=*/true);
	REQUIRE(v.size() == 2);
	CHECK(v[0] == "a"); CHECK(v[1] == "c");

	v.clear();
	split("a,,c", ",", v, /*bIgnoreEmpty=*/false);
	REQUIRE(v.size() == 3);
	CHECK(v[0] == "a"); CHECK(v[1] == ""); CHECK(v[2] == "c");

	v.clear();
	split(",a,", ",", v, /*bIgnoreEmpty=*/false);
	REQUIRE(v.size() == 3);
	CHECK(v[0] == ""); CHECK(v[1] == "a"); CHECK(v[2] == "");

	v.clear();
	split(",a,", ",", v, /*bIgnoreEmpty=*/true);
	REQUIRE(v.size() == 1);
	CHECK(v[0] == "a");
}

TEST_CASE("split on an empty source yields an empty vector regardless of bIgnoreEmpty", "[RageUtil][split]")
{
	std::vector<RString> v;
	split("", ",", v, false);
	CHECK(v.empty());
	split("", ",", v, true);
	CHECK(v.empty());
}

TEST_CASE("split on a multi-character delimiter", "[RageUtil][split]")
{
	std::vector<RString> v;
	split("aXXbXXc", "XX", v, true);
	REQUIRE(v.size() == 3);
	CHECK(v[0] == "a"); CHECK(v[1] == "b"); CHECK(v[2] == "c");
}

TEST_CASE("join is the inverse of split when empty fields are kept", "[RageUtil][split]")
{
	CHECK(join(",", std::vector<RString>{}) == "");
	CHECK(join(",", std::vector<RString>{ "a" }) == "a");
	CHECK(join(",", std::vector<RString>{ "a", "b", "c" }) == "a,b,c");
	CHECK(join("::", std::vector<RString>{ "x", "y" }) == "x::y");

	std::vector<RString> v;
	split("a,,c,d", ",", v, /*bIgnoreEmpty=*/false);
	CHECK(join(",", v) == "a,,c,d");
}

// Regex -- the PCRE wrapper used across simfile parsing, SongOptions,
// XmlFileUtil, etc. Note the capture-group indexing quirk: Compare()
// fills the out-vector from group 1 onward, so asMatches[0] is the
// FIRST parenthesised group, not the whole match.
TEST_CASE("Regex::Compare: whole-string match, true/false", "[RageUtil][regex]")
{
	Regex digits("^[0-9]+$");
	CHECK(digits.Compare("12345"));
	CHECK_FALSE(digits.Compare("12a45"));
	CHECK_FALSE(digits.Compare(""));

	Regex anywhere("ab");
	CHECK(anywhere.Compare("xxabxx")); // not anchored -> substring match
	CHECK_FALSE(anywhere.Compare("xxbaxx"));
}

TEST_CASE("Regex::Compare fills the out-vector with capture groups (index 0 == group 1)", "[RageUtil][regex]")
{
	// [.] is a literal dot without the backslash-escaping headaches.
	Regex ver("^([0-9]+)[.]([0-9]+)$");
	std::vector<RString> m;
	REQUIRE(ver.Compare("12.34", m));
	REQUIRE(m.size() == 2);
	CHECK(m[0] == "12"); // first () group
	CHECK(m[1] == "34"); // second () group

	m.clear();
	CHECK_FALSE(ver.Compare("12-34", m));
}

TEST_CASE("Regex::Replace substitutes the numbered placeholders with capture groups", "[RageUtil][regex]")
{
	// Replace() replaces literal "\${N}" tokens; \${0} maps to the first
	// capture group, \${1} to the second (Compare fills from group 1).
	Regex kv("^(.+)=(.+)$");
	RString out;
	REQUIRE(kv.Replace("\\${0}:\\${1}", "key=value", out));
	CHECK(out == "key:value");

	// No match -> Replace returns false and leaves out untouched.
	out = "unchanged";
	CHECK_FALSE(kv.Replace("x", "no equals here", out));
	CHECK(out == "unchanged");
}

TEST_CASE("Regex copy constructor keeps the compiled pattern usable", "[RageUtil][regex]")
{
	Regex orig("^hello");
	Regex copy(orig);
	CHECK(copy.Compare("hello world"));
	CHECK_FALSE(copy.Compare("say hello"));
}

// Capitalize / BeginsWith / EndsWith / URLEncode -- small string
// helpers with names that don't quite match what they do.
TEST_CASE("Capitalize upper-cases only the first codepoint", "[RageUtil][string]")
{
	// (RageUtil.cpp: Capitalize -> UnicodeDoUpper, which processes ONE
	// codepoint. MakeUpper is the whole-string one.)
	CHECK(Capitalize("hello world") == "Hello world");
	CHECK(Capitalize("MiXeD") == "MiXeD");   // first letter already upper -> unchanged
	CHECK(Capitalize("123abc") == "123abc"); // first char is a digit -> unchanged
	CHECK(Capitalize("") == "");
}

TEST_CASE("BeginsWith / EndsWith are plain substring anchors (case-sensitive)", "[RageUtil][string]")
{
	CHECK(BeginsWith("foobar", "foo"));
	CHECK_FALSE(BeginsWith("foobar", "Foo"));   // case-sensitive
	CHECK_FALSE(BeginsWith("foo", "foobar"));   // prefix longer than string
	CHECK(BeginsWith("foo", "foo"));            // whole string

	CHECK(EndsWith("foobar", "bar"));
	CHECK_FALSE(EndsWith("foobar", "Bar"));
	CHECK_FALSE(EndsWith("bar", "foobar"));
	CHECK(EndsWith("bar", "bar"));
}

TEST_CASE("URLEncode keeps printable ASCII '!'..'z' and %XX-escapes the rest", "[RageUtil][string]")
{
	CHECK(URLEncode("abcXYZ") == "abcXYZ");
	CHECK(URLEncode("a b") == "a%20b");          // space escaped
	CHECK(URLEncode("a\tb") == "a%09b");         // tab escaped
	// '[' '\' ']' '^' '_' '`' are all within '!'..'z' so they pass through.
	CHECK(URLEncode("[x]") == "[x]");
	// '{' '|' '}' are above 'z' -> escaped.
	CHECK(URLEncode("{x}") == "%7Bx%7D");
}
