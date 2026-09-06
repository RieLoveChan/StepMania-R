// Characterization tests that need the shared engine bootstrap fixture
// (EngineTestEnv): a live LOG, FILEMAN and LUA. These are the cases the
// note in test_NotesLoader.cpp called out as "out of scope -- LOG is
// null in this harness" plus a first end-to-end load of a committed
// simfile through the real file-read + tag-dispatch path.
//
// Still out of scope here (needs GAMEMAN, which EngineTestEnv does not
// construct): anything that parses a "#NOTES" / "#NOTEDATA" block --
// SMLoader::LoadFromTokens calls GAMEMAN->StringToStepsType. The corpus
// files under tests/data/ are therefore song-tags only. Growing the
// fixture to cover note data is the next step (see ADR 0006 phase 4 and
// backlog item 17).
//
// These pin CURRENT behaviour, bug-for-bug. A diff that changes an
// outcome here is a signal to stop and decide whether the change is
// intended -- not a licence to edit the expectation.
// See DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "MsdFile.h"
#include "NotesLoaderSM.h"
#include "NotesLoaderSSC.h"
#include "Song.h"
#include "TimingData.h"
#include "TimingSegments.h"

#include "catch_amalgamated.hpp"

#include <vector>
#include <utility>

using Catch::Approx;

// ---------------------------------------------------------------------------
// SMLoader::ParseBPMs / ParseStops -- the error branches that log
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

// ---------------------------------------------------------------------------
// Full song-tag load through the real MsdFile file read + dispatch map
// ---------------------------------------------------------------------------

TEST_CASE("SMLoader::LoadFromSimfile reads a committed .sm and fills song metadata", "[NotesLoader][SMLoader][load]")
{
	EngineTestEnv::Require();

	SMLoader loader;
	Song song;
	const RString path = EngineTestEnv::TestDataPath("characterization-basic.sm");

	REQUIRE(loader.LoadFromSimfile(path, song, /*bFromCache=*/false));

	CHECK(song.m_sMainTitle == "Characterization Basic");
	CHECK(song.m_sSubTitle == "The Subtitle");
	CHECK(song.m_sArtist == "Test Artist");
	CHECK(song.m_sMainTitleTranslit == "CharBasic");
	CHECK(song.m_sGenre == "Test");
	CHECK(song.m_sCredit == "test-suite");

	CHECK(song.m_SongTiming.m_fBeat0OffsetInSeconds == Approx(-0.123f));

	// #BPMS and #STOPS are collected during the tag loop and applied by
	// ProcessBPMsAndStops at the end of LoadFromSimfile.
	CHECK(song.m_SongTiming.GetBPMAtBeat(0.0f) == Approx(120.0f));
	CHECK(song.m_SongTiming.GetBPMAtBeat(4.0f) == Approx(120.0f));
	CHECK(song.m_SongTiming.GetBPMAtBeat(8.0f) == Approx(150.0f));
	CHECK(song.m_SongTiming.GetStopAtBeat(4.0f) == Approx(0.25f));
	CHECK(song.m_SongTiming.GetStopAtBeat(0.0f) == Approx(0.0f));

	// No #NOTES block in the corpus file.
	CHECK(song.GetAllSteps().empty());
}

TEST_CASE("SSCLoader::LoadFromSimfile reads a committed .ssc and fills song metadata", "[NotesLoader][SSCLoader][load]")
{
	EngineTestEnv::Require();

	SSCLoader loader;
	Song song;
	const RString path = EngineTestEnv::TestDataPath("characterization-basic.ssc");

	REQUIRE(loader.LoadFromSimfile(path, song, /*bFromCache=*/false));

	CHECK(song.m_fVersion == Approx(0.83f));
	CHECK(song.m_sMainTitle == "Characterization Basic SSC");
	CHECK(song.m_sSubTitle == "SSC Subtitle");
	CHECK(song.m_sArtist == "SSC Artist");
	CHECK(song.m_sGenre == "Test");
	CHECK(song.m_sCredit == "test-suite");

	CHECK(song.m_SongTiming.m_fBeat0OffsetInSeconds == Approx(-0.25f));

	// SSC applies #BPMS / #STOPS straight into m_SongTiming as the tag is
	// seen (ProcessBPMs / ProcessStops), unlike the deferred SM path.
	CHECK(song.m_SongTiming.GetBPMAtBeat(0.0f) == Approx(100.0f));
	CHECK(song.m_SongTiming.GetBPMAtBeat(16.0f) == Approx(200.0f));
	CHECK(song.m_SongTiming.GetStopAtBeat(8.0f) == Approx(0.5f));

	CHECK(song.GetAllSteps().empty());
}

TEST_CASE("A missing simfile path fails the load instead of crashing", "[NotesLoader][SMLoader][load][error]")
{
	EngineTestEnv::Require();

	SMLoader loader;
	Song song;
	// MsdFile::ReadFile returns false -> LoadFromSimfile logs and returns
	// false (this is the LOG->UserLog "couldn't be opened" branch).
	CHECK_FALSE(loader.LoadFromSimfile(EngineTestEnv::TestDataPath("does-not-exist.sm"), song, false));
}
