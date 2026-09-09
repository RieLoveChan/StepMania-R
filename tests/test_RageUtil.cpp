// Characterization tests for src/RageUtil string/path helpers.
//
// These pin the engine's CURRENT behaviour, bug-for-bug — including the
// quirks (see GetExtension with a slash after the dot, and Capitalize
// upper-casing the whole string). If a refactor changes an outcome here,
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
	// Only numeric inputs here: FromString<bool> routes a non-numeric
	// string through StringToInt() -> std::stoi(), whose invalid_argument
	// path logs via LOG->Warn -- which needs a live engine (see
	// test_IniFile / the EngineTestEnv-based suites). "true" therefore is
	// NOT recognised as a bool by this code, but exercising that needs
	// the fixture, so it is not asserted in this pure file.
	bool v = true;
	CHECK(StringConversion::FromString("0", v));  CHECK(v == false);
	CHECK(StringConversion::FromString("1", v));  CHECK(v == true);
	CHECK(StringConversion::FromString("5", v));  CHECK(v == true);
	CHECK(StringConversion::FromString("-3", v)); CHECK(v == true);
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
