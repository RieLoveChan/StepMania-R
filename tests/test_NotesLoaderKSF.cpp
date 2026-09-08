// Parse-regression for .ksf (Pump It Up / KSF format) via
// KSFLoader::GetApplicableFiles + LoadFromDir (ADR 0006 phase 4).
//
// The fixture is DERIVED, not a redistributed song:
// tests/data/Fixture Artist - KSF Fixture/ is a real 4-chart KSF set.
// Note data, #BPM, #TICKCOUNT, #STARTTIME, #DIFFICULTY and every #STEP
// block are kept byte-for-byte (diff-verified: only #TITLE / #ARTIST /
// #STEPMAKER / #SONGFILE differ from the source). KSF has no keysounds.
//
// KSF quirks the fixture layout has to respect (KSFLoader derives these
// from the FILENAME, lowercased):
//   * a name containing "double" -> pump_double + Difficulty_Medium
//   * a name with no difficulty keyword -> pump_single + Difficulty_Hard
//     (the else branch)
// so the files are named single-a/-b and double-a/-b. Meter comes from
// each file's #DIFFICULTY tag (kept). Title comes from #TITLE; artist
// comes from the *directory name* split on " - " (KSFLoader ignores
// #ARTIST), hence the "Fixture Artist - KSF Fixture" dir name.
//
// Pinned numbers are CHARACTERIZATION -- run `sm_tests "[ksfdump]"` to
// re-baseline. Verified against the untouched source folder: identical
// chart output, only the scrubbed strings differ.

#include "global.h"
#include "EngineTestEnv.h"

#include "NotesLoaderKSF.h"
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

	// tests/data/Fixture Artist - KSF Fixture/ -> /testdata/... .
	const char *kFixtureDir = "Fixture Artist - KSF Fixture/";

	// Captured from the "[ksfdump]" case (charts in GetApplicableFiles
	// order = filename-sorted: double-a, double-b, single-a, single-b).
	const char *kTitle  = "KSF Fixture";   // from #TITLE
	const char *kArtist = "Fixture Artist"; // from the dir name (KSFLoader ignores #ARTIST)
	const float kBpm0   = 220.0f;
	const float kOffset = -0.17f;           // #STARTTIME:17 -> -17/100
	const std::vector<ChartExpect> kCharts = {
		{ "pump-double", Difficulty_Medium, 17, 10, 601 },
		{ "pump-double", Difficulty_Medium, 26, 10, 895 },
		{ "pump-single", Difficulty_Hard,   17,  5, 622 },
		{ "pump-single", Difficulty_Hard,   23,  5, 807 },
	};

	bool LoadKsf( Song &song )
	{
		const RString dir = EngineTestEnv::TestDataPath( kFixtureDir );
		std::vector<RString> applicable;
		KSFLoader::GetApplicableFiles( dir, applicable );
		if( applicable.empty() )
			return false;
		song.SetSongDir( dir );          // KSFLoader has no Dirname fallback
		return KSFLoader::LoadFromDir( dir, song );
	}
}

TEST_CASE( "KSFLoader::LoadFromDir parses the derived .ksf fixture (4 charts, pump single/double)",
           "[NotesLoader][KSFLoader][ksf][regression]" )
{
	EngineTestEnv::Require();

	Song song;
	REQUIRE( LoadKsf( song ) );

	CHECK( song.m_sMainTitle == kTitle );
	CHECK( song.m_sArtist == kArtist );
	CHECK( song.m_SongTiming.GetBPMAtBeat( 0.0f ) == Approx( kBpm0 ) );
	CHECK( song.m_SongTiming.m_fBeat0OffsetInSeconds == Approx( kOffset ) );

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

// Hidden. Run with:  sm_tests "[ksfdump]"
TEST_CASE( "dump KSF fixture loader values", "[.][ksf][ksfdump]" )
{
	EngineTestEnv::Require();

	Song song;
	bool ok = LoadKsf( song );

	std::printf( "\n=== %s (loaded=%d)\n", kFixtureDir, ok ? 1 : 0 );
	std::printf( "    title=[%s] artist=[%s] bpm0=%.4f offset=%.4f charts=%d\n",
		song.m_sMainTitle.c_str(), song.m_sArtist.c_str(),
		song.m_SongTiming.GetBPMAtBeat( 0.0f ),
		song.m_SongTiming.m_fBeat0OffsetInSeconds,
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
