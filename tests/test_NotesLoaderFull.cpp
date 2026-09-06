// SMLoader parser error/edge branches that were out of scope in
// test_NotesLoader.cpp because they call LOG->UserLog and LOG was null
// in the bare harness. They need EngineTestEnv (a live LOG) but no
// files -- the parse primitives take strings directly.
//
// Full-file loading of real simfiles (metadata, timing, note counts,
// per-format equivalence) lives in test_NotesLoaderCorpus.cpp, which
// loads the committed SM5 sample songs. Toy simfiles are not used --
// see tests/data/README.md.
//
// These pin CURRENT behaviour, bug-for-bug. A diff that changes an
// outcome here is a stop-and-decide signal, not licence to edit the
// expectation. See DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "NotesLoaderSM.h"
#include "Song.h"

#include "catch_amalgamated.hpp"

#include <vector>
#include <utility>

using Catch::Approx;

// ---------------------------------------------------------------------------
// SMLoader::ParseBPMs / ParseStops -- the error branches that log and skip
// ---------------------------------------------------------------------------

TEST_CASE("ParseBPMs drops an expression that does not have exactly one '='", "[NotesLoader][SMLoader][bpm][error]")
{
	EngineTestEnv::Require(); // for LOG->UserLog inside the error branch

	SMLoader loader;
	std::vector<std::pair<float, float>> out;
	loader.ParseBPMs(out, "0=120,1=140=999,2=90");

	// The malformed middle expression is logged and skipped; the two
	// well-formed ones survive, in order.
	REQUIRE(out.size() == 2);
	CHECK(out[0].first == Approx(0.0f));
	CHECK(out[0].second == Approx(120.0f));
	CHECK(out[1].first == Approx(2.0f));
	CHECK(out[1].second == Approx(90.0f));
}

TEST_CASE("ParseBPMs drops a zero BPM entry", "[NotesLoader][SMLoader][bpm][error]")
{
	EngineTestEnv::Require();

	SMLoader loader;
	std::vector<std::pair<float, float>> out;
	loader.ParseBPMs(out, "0=120,4=0,8=150");

	REQUIRE(out.size() == 2);
	CHECK(out[0].second == Approx(120.0f));
	CHECK(out[1].first == Approx(8.0f));
	CHECK(out[1].second == Approx(150.0f));
}

TEST_CASE("ParseStops drops a malformed expression and a zero-length stop", "[NotesLoader][SMLoader][stop][error]")
{
	EngineTestEnv::Require();

	SMLoader loader;
	std::vector<std::pair<float, float>> out;
	loader.ParseStops(out, "2=0.5,3=0,4=1=1,6=0.25");

	// "3=0" -> zero-length, skipped. "4=1=1" -> three values, skipped.
	REQUIRE(out.size() == 2);
	CHECK(out[0].first == Approx(2.0f));
	CHECK(out[0].second == Approx(0.5f));
	CHECK(out[1].first == Approx(6.0f));
	CHECK(out[1].second == Approx(0.25f));
}

TEST_CASE("A missing simfile path fails the load instead of crashing", "[NotesLoader][SMLoader][load][error]")
{
	EngineTestEnv::Require();

	SMLoader loader;
	Song song;
	// MsdFile::ReadFile returns false -> LoadFromSimfile logs (LOG->UserLog
	// "couldn't be opened") and returns false.
	CHECK_FALSE(loader.LoadFromSimfile(
		EngineTestEnv::SongPath("no-such-song/no-such-file.sm"), song, false));
}
