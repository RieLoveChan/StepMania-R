// Characterization tests for the simfile-parsing primitives shared by the
// NotesLoader* family:
//
//   * MsdFile                                  -- the #TAG:param:param; tokenizer
//                                                 under every .sm/.ssc/.sma/.dwi
//   * NotesLoader::GetMainAndSubTitlesFromFullTitle
//   * SMLoader::RowToBeat / ParseBPMs / ParseStops
//   * SMLoader::ProcessBPMsAndStops / ProcessDelays / ProcessTimeSignatures
//     / ProcessTickcounts  (valid-input paths only -- see the note below)
//
// Full-file loading (SMLoader::LoadFromDir / LoadFromSimfile and the
// per-format LoadFromDir entry points) needs a live FILEMAN + LUA + more,
// which is smoke-test territory (--SelfTest), not a characterization
// unit. The AGENTS.md #5 invariant -- "every simfile format must keep
// loading with identical results, notes/timing/metadata" -- is guarded
// here at the parse-primitive level: these are the functions that turn
// simfile text into beats, rows and TimingData segments.
//
// Only the *valid-input* branch of the SMLoader helpers is exercised.
// Their error/edge branches (invalid "a=b=c" expressions, zero BPM,
// zero-length stop, negative beats) all call LOG->UserLog(), and LOG is
// null in this harness, so those are out of scope here.
//
// These pin the engine's CURRENT behaviour, bug-for-bug -- including the
// quirks (GetMainAndSubTitlesFromFullTitle keeps the non-space half of
// the separator in the subtitle; ProcessTimeSignatures back-fills a 4/4
// segment; ProcessTickcounts clamps to ROWS_PER_BEAT). If a refactor
// changes an outcome here, that is a signal to stop and decide whether
// the change is intended, not a licence to edit the expectation.
// See DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "MsdFile.h"
#include "NotesLoader.h"
#include "NotesLoaderSM.h"
#include "TimingData.h"
#include "TimingSegments.h"
#include "NoteTypes.h"

#include "catch_amalgamated.hpp"

using Catch::Approx;

// ---------------------------------------------------------------------------
// MsdFile -- the tag/param tokenizer
// ---------------------------------------------------------------------------

TEST_CASE("MsdFile splits #TAG:value; into a value with two params", "[NotesLoader][MsdFile]")
{
	MsdFile msd;
	msd.ReadFromString("#TITLE:My Song;\n#ARTIST:Some One;\n", false);

	REQUIRE(msd.GetNumValues() == 2);
	CHECK(msd.GetNumParams(0) == 2);
	CHECK(msd.GetParam(0, 0) == "TITLE");
	CHECK(msd.GetParam(0, 1) == "My Song");
	CHECK(msd.GetParam(1, 0) == "ARTIST");
	CHECK(msd.GetParam(1, 1) == "Some One");
}

TEST_CASE("MsdFile treats every colon as a param break", "[NotesLoader][MsdFile]")
{
	MsdFile msd;
	// A #NOTES header shape: 6 colons -> 7 params. The note body "0000,0000"
	// has no colon so it stays a single param.
	msd.ReadFromString("#NOTES:dance-single:Kyz:Challenge:10:0.9,0.8:0000,0000;", false);

	REQUIRE(msd.GetNumValues() == 1);
	CHECK(msd.GetNumParams(0) == 7);
	CHECK(msd.GetParam(0, 0) == "NOTES");
	CHECK(msd.GetParam(0, 1) == "dance-single");
	CHECK(msd.GetParam(0, 3) == "Challenge");
	CHECK(msd.GetParam(0, 6) == "0000,0000");
}

TEST_CASE("MsdFile recovers from a missing semicolon at the next line's #", "[NotesLoader][MsdFile]")
{
	MsdFile msd;
	// No ';' after "Foo" -- the '#' starting the next line ends the value.
	msd.ReadFromString("#TITLE:Foo\n#ARTIST:Bar;\n", false);

	REQUIRE(msd.GetNumValues() == 2);
	CHECK(msd.GetParam(0, 1) == "Foo");   // trailing newline stripped
	CHECK(msd.GetParam(1, 1) == "Bar");
}

TEST_CASE("MsdFile strips // comments outside of values", "[NotesLoader][MsdFile]")
{
	MsdFile msd;
	msd.ReadFromString("#TITLE:Foo;\n// a whole-line comment\n#ARTIST:Bar;\n", false);

	REQUIRE(msd.GetNumValues() == 2);
	CHECK(msd.GetParam(0, 1) == "Foo");
	CHECK(msd.GetParam(1, 1) == "Bar");
}

TEST_CASE("MsdFile with bUnescape keeps a backslash-escaped colon inside one param", "[NotesLoader][MsdFile]")
{
	MsdFile msd;
	// literal text: #TITLE:a\:b;   -- the \: must NOT split the param.
	msd.ReadFromString("#TITLE:a\\:b;", true);

	REQUIRE(msd.GetNumValues() == 1);
	CHECK(msd.GetNumParams(0) == 2);
	CHECK(msd.GetParam(0, 1) == "a:b");
}

TEST_CASE("MsdFile::GetParam returns empty for out-of-range indices", "[NotesLoader][MsdFile]")
{
	MsdFile msd;
	msd.ReadFromString("#TITLE:Foo;", false);

	CHECK(msd.GetParam(0, 5) == "");
	CHECK(msd.GetParam(9, 0) == "");
}

// ---------------------------------------------------------------------------
// NotesLoader::GetMainAndSubTitlesFromFullTitle
// ---------------------------------------------------------------------------

TEST_CASE("GetMainAndSubTitlesFromFullTitle splits on ' (' and drops only the space", "[NotesLoader][title]")
{
	RString main, sub;
	NotesLoader::GetMainAndSubTitlesFromFullTitle("Song Name (Extended Mix)", main, sub);

	CHECK(main == "Song Name");
	CHECK(sub == "(Extended Mix)"); // the '(' half of the separator stays in the subtitle
}

TEST_CASE("GetMainAndSubTitlesFromFullTitle handles the tab, dash, tilde and bracket separators", "[NotesLoader][title]")
{
	RString main, sub;

	NotesLoader::GetMainAndSubTitlesFromFullTitle("Song\tSub", main, sub);
	CHECK(main == "Song");
	CHECK(sub == "Sub");

	NotesLoader::GetMainAndSubTitlesFromFullTitle("Song -Remix", main, sub);
	CHECK(main == "Song");
	CHECK(sub == "-Remix");

	NotesLoader::GetMainAndSubTitlesFromFullTitle("Song ~subtitle~", main, sub);
	CHECK(main == "Song");
	CHECK(sub == "~subtitle~");

	NotesLoader::GetMainAndSubTitlesFromFullTitle("Song [hard]", main, sub);
	CHECK(main == "Song");
	CHECK(sub == "[hard]");
}

TEST_CASE("GetMainAndSubTitlesFromFullTitle checks the tab separator before ' -'", "[NotesLoader][title]")
{
	RString main, sub;
	NotesLoader::GetMainAndSubTitlesFromFullTitle("Song\tThing -x", main, sub);

	CHECK(main == "Song");
	CHECK(sub == "Thing -x"); // tab matched first, so the ' -' is left in the subtitle
}

TEST_CASE("GetMainAndSubTitlesFromFullTitle leaves a title with no separator alone", "[NotesLoader][title]")
{
	RString main, sub;
	NotesLoader::GetMainAndSubTitlesFromFullTitle("PlainTitle", main, sub);

	CHECK(main == "PlainTitle");
	CHECK(sub == "");
}

// ---------------------------------------------------------------------------
// SMLoader::RowToBeat
// ---------------------------------------------------------------------------

TEST_CASE("RowToBeat returns the number as-is when there is no r/R suffix", "[NotesLoader][SMLoader][row]")
{
	SMLoader loader;
	CHECK(loader.RowToBeat("4.5", 48) == Approx(4.5f));
	CHECK(loader.RowToBeat("0", -1) == Approx(0.0f));
}

TEST_CASE("RowToBeat divides by rowsPerBeat when the value carries an r or R suffix", "[NotesLoader][SMLoader][row]")
{
	SMLoader loader;
	CHECK(loader.RowToBeat("192r", 48) == Approx(4.0f));
	CHECK(loader.RowToBeat("96R", 48) == Approx(2.0f));
}

// ---------------------------------------------------------------------------
// SMLoader::ParseBPMs / ParseStops  (valid input only)
// ---------------------------------------------------------------------------

TEST_CASE("ParseBPMs splits on ',' then '=' into (beat, bpm) pairs", "[NotesLoader][SMLoader][bpm]")
{
	SMLoader loader;
	std::vector<std::pair<float, float>> out;
	loader.ParseBPMs(out, "0=120,4=140,8=90");

	REQUIRE(out.size() == 3);
	CHECK(out[0].first == Approx(0.0f));
	CHECK(out[0].second == Approx(120.0f));
	CHECK(out[1].first == Approx(4.0f));
	CHECK(out[1].second == Approx(140.0f));
	CHECK(out[2].second == Approx(90.0f));
}

TEST_CASE("ParseBPMs applies the r-suffix row conversion to the beat field", "[NotesLoader][SMLoader][bpm]")
{
	SMLoader loader;
	std::vector<std::pair<float, float>> out;
	loader.ParseBPMs(out, "0=120,192r=200", 48);

	REQUIRE(out.size() == 2);
	CHECK(out[1].first == Approx(4.0f)); // 192 rows / 48 rows-per-beat
	CHECK(out[1].second == Approx(200.0f));
}

TEST_CASE("ParseStops splits into (beat, seconds) pairs", "[NotesLoader][SMLoader][stop]")
{
	SMLoader loader;
	std::vector<std::pair<float, float>> out;
	loader.ParseStops(out, "2=0.5,6=1.25");

	REQUIRE(out.size() == 2);
	CHECK(out[0].first == Approx(2.0f));
	CHECK(out[0].second == Approx(0.5f));
	CHECK(out[1].first == Approx(6.0f));
	CHECK(out[1].second == Approx(1.25f));
}

// ---------------------------------------------------------------------------
// SMLoader::Process*  -> TimingData  (valid input only)
// ---------------------------------------------------------------------------

TEST_CASE("ProcessBPMsAndStops adds an initial BPM segment at row 0", "[NotesLoader][SMLoader][timing]")
{
	SMLoader loader;
	TimingData timing;
	std::vector<std::pair<float, float>> bpms{ {0.0f, 150.0f} };
	std::vector<std::pair<float, float>> stops;

	loader.ProcessBPMsAndStops(timing, bpms, stops);

	REQUIRE(timing.GetTimingSegments(SEGMENT_BPM).size() == 1);
	CHECK(timing.GetBPMAtRow(0) == Approx(150.0f));
}

TEST_CASE("ProcessBPMsAndStops folds a pre-zero-beat stop into the song offset", "[NotesLoader][SMLoader][timing]")
{
	SMLoader loader;
	TimingData timing; // offset starts at 0
	std::vector<std::pair<float, float>> bpms{ {0.0f, 120.0f} };
	std::vector<std::pair<float, float>> stops{ {-1.0f, 0.75f} };

	loader.ProcessBPMsAndStops(timing, bpms, stops);

	// A positive stop before beat 0 subtracts from the offset.
	CHECK(timing.m_fBeat0OffsetInSeconds == Approx(-0.75f));
	CHECK(timing.GetTimingSegments(SEGMENT_STOP).empty()); // not kept as a stop segment
}

TEST_CASE("ProcessDelays adds a delay segment for a positive-length entry", "[NotesLoader][SMLoader][timing]")
{
	SMLoader loader;
	TimingData timing;
	loader.ProcessDelays(timing, "1=0.25");

	REQUIRE(timing.HasDelays());
	CHECK(timing.GetDelayAtBeat(1.0f) == Approx(0.25f));
}

TEST_CASE("ProcessTimeSignatures back-fills a 4/4 segment when the first entry is not at beat 0", "[NotesLoader][SMLoader][timing]")
{
	SMLoader loader;

	SECTION("first entry already at beat 0 -> single segment")
	{
		TimingData timing;
		loader.ProcessTimeSignatures(timing, "0=3=4");
		CHECK(timing.GetTimingSegments(SEGMENT_TIME_SIG).size() == 1);
	}

	SECTION("first entry at beat 4 -> an implicit 4/4 is prepended")
	{
		TimingData timing;
		loader.ProcessTimeSignatures(timing, "4=7=8");
		CHECK(timing.GetTimingSegments(SEGMENT_TIME_SIG).size() == 2);
	}
}

TEST_CASE("ProcessTickcounts clamps the tick value to [0, ROWS_PER_BEAT]", "[NotesLoader][SMLoader][timing]")
{
	SMLoader loader;
	TimingData timing;
	loader.ProcessTickcounts(timing, "0=2,4=9999");

	CHECK(timing.GetTickcountAtBeat(0.0f) == 2);
	CHECK(timing.GetTickcountAtBeat(4.0f) == ROWS_PER_BEAT); // 9999 clamped to 48
}
