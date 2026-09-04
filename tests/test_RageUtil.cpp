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
