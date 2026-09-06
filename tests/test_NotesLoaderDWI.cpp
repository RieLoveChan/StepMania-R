// Parse-regression for .dwi via DWILoader::LoadFromDir (ADR 0006 phase 4).
//
// The fixture is DERIVED, not a redistributed song: tests/data/dwi-fixture/
// is a real 3-chart .dwi (community simfile, not clearly redistributable)
// with the note data, #BPM, #GAP, #CHANGEBPM, #SAMPLESTART, #RANDSTART and
// all three #SINGLE blocks kept byte-for-byte -- verified with a diff, only
// #FILE / #TITLE / #ARTIST differ from the source. DWI has no keysounds, so
// nothing else to stub. See tests/data/README.md.
//
// Pinned numbers are CHARACTERIZATION -- run `sm_tests "[dwidump]"` to
// re-baseline. A diff that moves one is a stop-and-decide signal.

#include "global.h"
#include "EngineTestEnv.h"

#include "NotesLoaderDWI.h"
#include "Song.h"
#include "Steps.h"
#include "NoteData.h"
#include "TimingData.h"
#include "GameManager.h"
#include "Difficulty.h"

#include "catch_amalgamated.hpp"

#include <cstdio>
#include <set>
#include <vector>

using Catch::Approx;

namespace
{
	struct ChartExpect
	{
		const char *stepsTypeStr;
		Difficulty  difficulty;
		int         meter;
		int         numTracks;
		int         numTapsNoTiming;
	};

	// tests/data/dwi-fixture/ -> /testdata/dwi-fixture/ .
	const char *kFixtureDir = "dwi-fixture/";

	// Captured from the "[dwidump]" case.
	// #SINGLE:BASIC:3 / ANOTHER:8 / MANIAC:10  ->  Easy / Medium / Hard.
	const std::vector<ChartExpect> kCharts = {
		{ "dance-single", Difficulty_Easy,   3, 4, 233 },
		{ "dance-single", Difficulty_Medium, 8, 4, 443 },
		{ "dance-single", Difficulty_Hard,  10, 4, 680 },
	};
}

TEST_CASE( "DWILoader::LoadFromDir parses the derived .dwi fixture (3 charts, BPM change)",
           "[NotesLoader][DWILoader][dwi][regression]" )
{
	EngineTestEnv::Require();

	const RString dir = EngineTestEnv::TestDataPath( kFixtureDir );

	std::vector<RString> applicable;
	DWILoader::GetApplicableFiles( dir, applicable );
	REQUIRE( applicable.size() == 1 );

	Song song;
	std::set<RString> blacklist;
	REQUIRE( DWILoader::LoadFromDir( dir, song, blacklist ) );

	// #TITLE "DWI Fixture (Derived)" -> the DWI loader has no native
	// #SUBTITLE, so GetMainAndSubTitlesFromFullTitle splits on " (".
	CHECK( song.m_sMainTitle == "DWI Fixture" );
	CHECK( song.m_sSubTitle == "(Derived)" );
	CHECK( song.m_sArtist == "Test Fixture" );
	CHECK( song.m_sMusicFile == "fixture.mp3" );

	// #GAP:065 -> -65 ms.
	CHECK( song.m_SongTiming.m_fBeat0OffsetInSeconds == Approx( -0.065f ) );
	// #BPM:145 at row 0; #CHANGEBPM:1024=72.5 -> DWI row/4 -> beat 256.
	CHECK( song.m_SongTiming.GetBPMAtBeat( 0.0f ) == Approx( 145.0f ) );
	CHECK( song.m_SongTiming.GetBPMAtBeat( 256.0f ) == Approx( 72.5f ) );
	// #SAMPLESTART:79.485 (plain seconds).
	CHECK( song.m_fMusicSampleStartSeconds == Approx( 79.485f ) );

	auto const &steps = song.GetAllSteps();
	REQUIRE( steps.size() == kCharts.size() );

	for( size_t i = 0; i < kCharts.size(); ++i )
	{
		ChartExpect const &ce = kCharts[i];
		Steps *s = steps[i];
		CAPTURE( i );

		CHECK( s->m_StepsTypeStr == ce.stepsTypeStr );
		CHECK( s->m_StepsType == GAMEMAN->StringToStepsType( ce.stepsTypeStr ) );
		CHECK( s->GetDifficulty() == ce.difficulty );
		CHECK( s->GetMeter() == ce.meter );

		NoteData nd;
		s->GetNoteData( nd );
		CHECK( nd.GetNumTracks() == ce.numTracks );
		CHECK( nd.GetNumTapNotesNoTiming() == ce.numTapsNoTiming );
	}
}

// Hidden. Run with:  sm_tests "[dwidump]"
TEST_CASE( "dump DWI fixture loader values", "[.][dwi][dwidump]" )
{
	EngineTestEnv::Require();

	const RString dir = EngineTestEnv::TestDataPath( kFixtureDir );
	std::vector<RString> applicable;
	DWILoader::GetApplicableFiles( dir, applicable );

	Song song;
	std::set<RString> blacklist;
	bool ok = !applicable.empty() && DWILoader::LoadFromDir( dir, song, blacklist );

	std::printf( "\n=== %s (applicable=%d loaded=%d)\n",
		kFixtureDir, (int)applicable.size(), ok ? 1 : 0 );
	std::printf( "    title=[%s] subtitle=[%s] artist=[%s] music=[%s]\n",
		song.m_sMainTitle.c_str(), song.m_sSubTitle.c_str(),
		song.m_sArtist.c_str(), song.m_sMusicFile.c_str() );
	std::printf( "    offset=%.5f bpm@0=%.4f bpm@256=%.4f sampleStart=%.4f charts=%d\n",
		song.m_SongTiming.m_fBeat0OffsetInSeconds,
		song.m_SongTiming.GetBPMAtBeat( 0.0f ),
		song.m_SongTiming.GetBPMAtBeat( 256.0f ),
		song.m_fMusicSampleStartSeconds,
		(int)song.GetAllSteps().size() );
	for( Steps *s : song.GetAllSteps() )
	{
		NoteData nd;
		s->GetNoteData( nd );
		std::printf( "    { \"%s\", Difficulty_%s, %d, %d, %d },\n",
			s->m_StepsTypeStr.c_str(),
			DifficultyToString( s->GetDifficulty() ).c_str(),
			s->GetMeter(), nd.GetNumTracks(), nd.GetNumTapNotesNoTiming() );
	}
	SUCCEED( "dump complete" );
}
