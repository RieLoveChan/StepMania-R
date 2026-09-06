// Parse-regression over a committed simfile corpus (ADR 0006 phase 4).
//
// This is the AGENTS.md #5 invariant in test form: "every simfile format
// the engine loads today must keep loading, with identical results --
// notes, timing, metadata". Each file under tests/data/corpus/ is loaded
// through its real loader and a hand-computed set of outputs is pinned.
// A diff that changes any pinned value is a stop-and-decide signal.
//
// Scope: .sm / .ssc (the canonical read + canonical write formats), via
// LoadFromSimfile. The fixture provides LUA + FILEMAN + LOG + GAMEMAN,
// which is everything those two entry points touch. The DWI / KSF / BMS
// loaders additionally read PREFSMAN (e.g. DWILoader checks
// m_bQuirksMode) and only expose LoadFromDir -- adding them needs
// PREFSMAN in EngineTestEnv and per-format directory fixtures; tracked
// as the remaining phase-4 work in modernization-backlog.md item 17.
//
// See DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "NotesLoaderSM.h"
#include "NotesLoaderSSC.h"
#include "Song.h"
#include "Steps.h"
#include "NoteData.h"
#include "TimingData.h"
#include "GameManager.h"
#include "Difficulty.h"

#include "catch_amalgamated.hpp"

#include <cstring>
#include <vector>

using Catch::Approx;
using Catch::Generators::from_range;

namespace
{
	struct ChartExpect
	{
		const char *stepsTypeStr;
		Difficulty  difficulty;
		int         meter;
		int         numTracks;
		int         numTapsNoTiming; // TapNoteType_Tap only -- not holds/mines
	};

	struct CorpusExpect
	{
		const char *file;   // relative to /testdata
		bool        ssc;    // false -> SMLoader, true -> SSCLoader
		const char *mainTitle;
		const char *subTitle;
		const char *artist;
		float       offset;
		float       songBpmAtBeat0;
		std::vector<ChartExpect> charts; // in file order
	};

	// Values here are hand-computed from the corpus files. Keep the two
	// in sync -- the test is the pin, the file is the fixture.
	const std::vector<CorpusExpect> kCorpus = {
		{ "corpus/corpus-a.sm", false, "Corpus A", "sm single", "Tester",
		  -0.009f, 120.0f,
		  { { "dance-single", Difficulty_Easy, 3, 4, 4 } } },

		{ "corpus/corpus-b.sm", false, "Corpus B", "sm two charts", "Tester",
		  0.0f, 100.0f,
		  { { "dance-single", Difficulty_Easy,      2, 4, 2 },
		    { "dance-single", Difficulty_Challenge, 9, 4, 8 } } },

		{ "corpus/corpus-c.ssc", true, "Corpus C", "ssc split timing", "Tester",
		  0.0f, 120.0f,
		  { { "dance-single", Difficulty_Medium, 5, 4, 3 } } },
	};

	bool LoadCorpusFile( const CorpusExpect &e, Song &song )
	{
		const RString path = EngineTestEnv::TestDataPath( e.file );
		if( e.ssc )
		{
			SSCLoader loader;
			return loader.LoadFromSimfile( path, song, /*bFromCache=*/false );
		}
		SMLoader loader;
		return loader.LoadFromSimfile( path, song, /*bFromCache=*/false );
	}
}

TEST_CASE( "Committed simfile corpus loads with pinned metadata, timing and note counts",
           "[NotesLoader][corpus][regression]" )
{
	EngineTestEnv::Require();

	auto const &e = GENERATE( from_range( kCorpus ) );
	CAPTURE( e.file );

	Song song;
	REQUIRE( LoadCorpusFile( e, song ) );

	CHECK( song.m_sMainTitle == e.mainTitle );
	CHECK( song.m_sSubTitle == e.subTitle );
	CHECK( song.m_sArtist == e.artist );
	CHECK( song.m_SongTiming.m_fBeat0OffsetInSeconds == Approx( e.offset ) );
	CHECK( song.m_SongTiming.GetBPMAtBeat( 0.0f ) == Approx( e.songBpmAtBeat0 ) );

	auto const &steps = song.GetAllSteps();
	REQUIRE( steps.size() == e.charts.size() );

	for( size_t i = 0; i < e.charts.size(); ++i )
	{
		ChartExpect const &ce = e.charts[i];
		Steps *s = steps[i];
		CAPTURE( i, ce.stepsTypeStr );

		CHECK( s->m_StepsType == GAMEMAN->StringToStepsType( ce.stepsTypeStr ) );
		CHECK( s->m_StepsTypeStr == ce.stepsTypeStr );
		CHECK( s->GetDifficulty() == ce.difficulty );
		CHECK( s->GetMeter() == ce.meter );

		NoteData nd;
		s->GetNoteData( nd );
		CHECK( nd.GetNumTracks() == ce.numTracks );
		CHECK( nd.GetNumTapNotesNoTiming() == ce.numTapsNoTiming );
	}
}

TEST_CASE( "SSC split timing keeps the chart BPM distinct from the song BPM",
           "[NotesLoader][SSCLoader][corpus][timing]" )
{
	EngineTestEnv::Require();

	Song song;
	SSCLoader loader;
	REQUIRE( loader.LoadFromSimfile(
		EngineTestEnv::TestDataPath( "corpus/corpus-c.ssc" ), song, false ) );
	REQUIRE( song.GetAllSteps().size() == 1 );

	Steps *s = song.GetAllSteps()[0];
	// The #NOTEDATA block carried its own #BPMS (180); with #VERSION >=
	// VERSION_SPLIT_TIMING that populates the chart's own m_Timing, which
	// then wins over the song timing (120).
	CHECK_FALSE( s->m_Timing.empty() );
	CHECK( s->GetTimingData()->GetBPMAtBeat( 0.0f ) == Approx( 180.0f ) );
	CHECK( song.m_SongTiming.GetBPMAtBeat( 0.0f ) == Approx( 120.0f ) );
}
