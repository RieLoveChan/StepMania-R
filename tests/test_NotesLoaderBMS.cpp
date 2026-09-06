// Parse-regression for the BMS family via BMSLoader::LoadFromDir
// (ADR 0006 phase 4). Covers .pms (Pop'n Music) with keysounds.
//
// The fixture is DERIVED, not a redistributed song: tests/data/pms-fixture/
// is a real 3-chart .pms set (5-button / battle / normal layouts) with its
// note data, timing, BPM changes and keysound-channel structure intact,
// but #TITLE / #ARTIST / #GENRE and every #WAV filename replaced with
// generic placeholders, and the keysound audio replaced with 44-byte
// silent stub WAVs. The real Pop'n Music song it came from is Konami's
// and not redistributable; what the parser actually exercises (channels,
// #WAVxx -> index mapping, note rows, #BPM/#BPMxx) is unchanged. See
// tests/data/README.md.
//
// Pinned numbers are CHARACTERIZATION -- run `sm_tests "[bmsdump]"` to
// re-baseline. A diff that moves one is a stop-and-decide signal.

#include "global.h"
#include "EngineTestEnv.h"

#include "NotesLoaderBMS.h"
#include "Song.h"
#include "Steps.h"
#include "NoteData.h"
#include "TimingData.h"
#include "GameManager.h"
#include "Difficulty.h"

#include "catch_amalgamated.hpp"

#include <cstdio>
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

	// tests/data/pms-fixture/ -> /testdata/pms-fixture/ .  LoadFromDir
	// wants a trailing slash and picks up every .pms in the directory.
	const char *kFixtureDir = "pms-fixture/";

	// Captured from the "[bmsdump]" case (charts in GetApplicableFiles
	// order, which is filename-sorted: fixture-5button, -battle, -normal).
	// #WAV defs: 62 per file; 54 distinct entries end up referenced and
	// land in m_vsKeysoundFile (8 are defined but unused; the three files
	// share the pool). All 62 stub WAVs exist -> no "can't be found".
	const char *kTitle  = "PMS Keysound Fixture";
	const char *kArtist = "Test Fixture";
	const float kBpm0   = 107.0f;
	const int   kKeysoundFiles = 54;
	const std::vector<ChartExpect> kCharts = {
		{ "pnm-five",   Difficulty_Hard,   7,  5, 118 },
		{ "bm-double7", Difficulty_Easy,   1, 16, 118 },
		{ "pnm-nine",   Difficulty_Medium, 6,  9, 118 },
	};
}

TEST_CASE( "BMSLoader::LoadFromDir parses the derived .pms fixture (3 charts, keysounds)",
           "[NotesLoader][BMSLoader][bms][pms][regression]" )
{
	EngineTestEnv::Require();

	const RString dir = EngineTestEnv::TestDataPath( kFixtureDir );

	std::vector<RString> applicable;
	BMSLoader::GetApplicableFiles( dir, applicable );
	REQUIRE( applicable.size() == 3 );

	Song song;
	REQUIRE( BMSLoader::LoadFromDir( dir, song ) );

	CHECK( song.m_sMainTitle == kTitle );
	CHECK( song.m_sArtist == kArtist );
	CHECK( song.m_SongTiming.GetBPMAtBeat( 0.0f ) == Approx( kBpm0 ) );

	// The #WAV filenames all resolve to the committed silent stub WAVs.
	CHECK( (int)song.m_vsKeysoundFile.size() == kKeysoundFiles );

	auto const &steps = song.GetAllSteps();
	REQUIRE( steps.size() == kCharts.size() );

	for( size_t i = 0; i < kCharts.size(); ++i )
	{
		ChartExpect const &ce = kCharts[i];
		Steps *s = steps[i];
		CAPTURE( i, ce.stepsTypeStr );

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

// Hidden. Run with:  sm_tests "[bmsdump]"
TEST_CASE( "dump BMS .pms fixture loader values", "[.][bms][bmsdump]" )
{
	EngineTestEnv::Require();

	const RString dir = EngineTestEnv::TestDataPath( kFixtureDir );
	std::vector<RString> applicable;
	BMSLoader::GetApplicableFiles( dir, applicable );

	Song song;
	bool ok = !applicable.empty() && BMSLoader::LoadFromDir( dir, song );

	std::printf( "\n=== %s (applicable=%d loaded=%d)\n",
		kFixtureDir, (int)applicable.size(), ok ? 1 : 0 );
	std::printf( "    title=[%s] subtitle=[%s] artist=[%s] genre=[%s] bpm0=%.4f\n",
		song.m_sMainTitle.c_str(), song.m_sSubTitle.c_str(), song.m_sArtist.c_str(),
		song.m_sGenre.c_str(), song.m_SongTiming.GetBPMAtBeat( 0.0f ) );
	std::printf( "    keysoundfiles=%d  charts=%d\n",
		(int)song.m_vsKeysoundFile.size(), (int)song.GetAllSteps().size() );
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
