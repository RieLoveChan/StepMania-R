// Parse-regression over the real committed sample songs (ADR 0006
// phase 4). This is the AGENTS.md #5 invariant in test form: "every
// simfile format the engine loads today must keep loading, with
// identical results -- notes, timing, metadata."
//
// Toy simfiles are deliberately not used -- see tests/data/README.md.
// The fixtures are the SM5 sample songs under Songs/StepMania 5/, loaded
// via EngineTestEnv (LUA + FILEMAN + LOG + GAMEMAN) through each format's
// real LoadFromSimfile entry point.
//
// Every pinned number is CHARACTERIZATION: it is whatever the loader
// produces today, captured (see the hidden "[.dump]" case at the bottom,
// run it with `sm_tests "[dump]"` to re-baseline). A diff that moves any
// of these is a stop-and-decide signal, not licence to edit the number.
//
// Not covered here (backlog item 17): .sma / .dwi / .ksf / .bms / .crs
// -- no committed sample, and the dir-only loaders additionally need
// PREFSMAN in EngineTestEnv.

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

#include <cstdio>
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
		int         numTapsNoTiming;
	};

	struct SongExpect
	{
		const char *songPath;   // relative to /Songs
		bool        ssc;
		const char *mainTitle;
		const char *subTitle;
		const char *artist;
		float       offset;
		float       bpmAtBeat0;
		std::vector<ChartExpect> charts; // file order
	};

	bool LoadSong( const SongExpect &e, Song &song )
	{
		const RString path = EngineTestEnv::SongPath( e.songPath );
		if( e.ssc )
		{
			SSCLoader loader;
			return loader.LoadFromSimfile( path, song, /*bFromCache=*/false );
		}
		SMLoader loader;
		return loader.LoadFromSimfile( path, song, /*bFromCache=*/false );
	}

	// Captured from the "[.dump]" case (charts in file order). Track
	// counts are fixed by GAMEMAN: dance-single 4, dance-double 8,
	// dance-solo 6, dance-couple 8, dance-threepanel 3, pump-single 5,
	// pump-halfdouble 6. The tap counts are GetNumTapNotesNoTiming()
	// as the loader produces them today.
	//
	// Note: Springtime's charts trip "Unmatched 3" warnings from
	// NoteDataUtil::LoadFromSMNoteDataString (hold tails with no head) --
	// that is real current behaviour; the loader tolerates them and the
	// counts below already account for it.
	const std::vector<SongExpect> kCorpus = {
		{ "StepMania 5/Goin' Under/Goin' Under.sm", false,
		  "Goin' Under", "", "NegaRen", 0.0f, 210.0f,
		  {
		    { "dance-single", Difficulty_Challenge, 10, 4, 642 },
		    { "dance-single", Difficulty_Hard,       8, 4, 472 },
		    { "dance-single", Difficulty_Medium,     6, 4, 352 },
		    { "dance-single", Difficulty_Easy,       3, 4, 220 },
		    { "dance-single", Difficulty_Beginner,   1, 4,  95 },
		    { "dance-double", Difficulty_Challenge, 11, 8, 649 },
		    { "dance-double", Difficulty_Hard,       9, 8, 472 },
		    { "dance-double", Difficulty_Medium,     6, 8, 350 },
		    { "dance-double", Difficulty_Easy,       3, 8, 217 },
		  } },
		{ "StepMania 5/Goin' Under/Goin' Under.ssc", true,
		  "Goin' Under", "", "NegaRen", 0.0f, 210.0f,
		  {
		    { "dance-single", Difficulty_Challenge, 10, 4, 642 },
		    { "dance-single", Difficulty_Hard,       8, 4, 472 },
		    { "dance-single", Difficulty_Medium,     6, 4, 352 },
		    { "dance-single", Difficulty_Easy,       3, 4, 220 },
		    { "dance-single", Difficulty_Beginner,   1, 4,  95 },
		    { "dance-double", Difficulty_Challenge, 11, 8, 649 },
		    { "dance-double", Difficulty_Hard,       9, 8, 472 },
		    { "dance-double", Difficulty_Medium,     6, 8, 350 },
		    { "dance-double", Difficulty_Easy,       3, 8, 217 },
		  } },
		{ "StepMania 5/MechaTribe Assault/Mecha-Tribe Assault.ssc", true,
		  "Mecha-Tribe Assault", "", "Kommisar", -0.028f, 180.0f,
		  {
		    { "dance-single",     Difficulty_Challenge, 11, 4, 766 },
		    { "dance-single",     Difficulty_Hard,      10, 4, 544 },
		    { "dance-single",     Difficulty_Medium,     8, 4, 395 },
		    { "dance-single",     Difficulty_Easy,       6, 4, 304 },
		    { "dance-single",     Difficulty_Beginner,   2, 4, 179 },
		    { "dance-solo",       Difficulty_Hard,      10, 6, 494 },
		    { "pump-single",      Difficulty_Hard,      19, 5, 738 },
		    { "pump-single",      Difficulty_Medium,    13, 5, 610 },
		    { "pump-single",      Difficulty_Easy,       6, 5, 316 },
		    { "dance-couple",     Difficulty_Easy,       3, 8, 284 },
		    { "dance-threepanel", Difficulty_Easy,       2, 3, 106 },
		    { "pump-halfdouble",  Difficulty_Medium,    11, 6, 375 },
		  } },
		{ "StepMania 5/Springtime/Springtime.ssc", true,
		  "Springtime", "", "Kommisar", -0.090f, 181.685f,
		  {
		    { "dance-single", Difficulty_Challenge, 12, 4,  939 },
		    { "dance-single", Difficulty_Hard,       9, 4,  675 },
		    { "dance-single", Difficulty_Beginner,   1, 4,  211 },
		    { "dance-single", Difficulty_Medium,     6, 4,  530 },
		    { "dance-single", Difficulty_Easy,       4, 4,  412 },
		    { "pump-single",  Difficulty_Medium,    10, 5,  769 },
		    { "pump-single",  Difficulty_Easy,       7, 5,  504 },
		    { "pump-single",  Difficulty_Challenge, 21, 5, 1086 },
		    { "pump-single",  Difficulty_Hard,      16, 5,  890 },
		  } },
	};
}

TEST_CASE( "SM5 sample songs load with pinned metadata, timing and per-chart note counts",
           "[NotesLoader][corpus][regression]" )
{
	EngineTestEnv::Require();

	auto const &e = GENERATE( from_range( kCorpus ) );
	CAPTURE( e.songPath );

	Song song;
	REQUIRE( LoadSong( e, song ) );

	CHECK( song.m_sMainTitle == e.mainTitle );
	CHECK( song.m_sSubTitle == e.subTitle );
	CHECK( song.m_sArtist == e.artist );
	CHECK( song.m_SongTiming.m_fBeat0OffsetInSeconds == Approx( e.offset ) );
	CHECK( song.m_SongTiming.GetBPMAtBeat( 0.0f ) == Approx( e.bpmAtBeat0 ) );

	auto const &steps = song.GetAllSteps();
	REQUIRE( steps.size() == e.charts.size() );

	for( size_t i = 0; i < e.charts.size(); ++i )
	{
		ChartExpect const &ce = e.charts[i];
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

TEST_CASE( "Goin' Under .sm and .ssc parse to the same charts and note counts",
           "[NotesLoader][corpus][equivalence]" )
{
	EngineTestEnv::Require();

	Song sm, ssc;
	{
		SMLoader l;
		REQUIRE( l.LoadFromSimfile(
			EngineTestEnv::SongPath( "StepMania 5/Goin' Under/Goin' Under.sm" ), sm, false ) );
	}
	{
		SSCLoader l;
		REQUIRE( l.LoadFromSimfile(
			EngineTestEnv::SongPath( "StepMania 5/Goin' Under/Goin' Under.ssc" ), ssc, false ) );
	}

	CHECK( sm.m_sMainTitle == ssc.m_sMainTitle );
	CHECK( sm.m_sArtist == ssc.m_sArtist );

	auto const &a = sm.GetAllSteps();
	auto const &b = ssc.GetAllSteps();
	REQUIRE( a.size() == b.size() );

	for( size_t i = 0; i < a.size(); ++i )
	{
		CAPTURE( i );
		CHECK( a[i]->m_StepsType == b[i]->m_StepsType );
		CHECK( a[i]->GetDifficulty() == b[i]->GetDifficulty() );
		CHECK( a[i]->GetMeter() == b[i]->GetMeter() );

		NoteData na, nb;
		a[i]->GetNoteData( na );
		b[i]->GetNoteData( nb );
		CHECK( na.GetNumTracks() == nb.GetNumTracks() );
		CHECK( na.GetNumTapNotesNoTiming() == nb.GetNumTapNotesNoTiming() );
	}
}

// Hidden. Run with:  sm_tests "[dump]"
// Prints the values to paste into kCorpus above / to re-baseline.
TEST_CASE( "dump SM5 sample-song loader values", "[.][corpus][dump]" )
{
	EngineTestEnv::Require();

	struct Item { const char *path; bool ssc; };
	const Item items[] = {
		{ "StepMania 5/Goin' Under/Goin' Under.sm", false },
		{ "StepMania 5/Goin' Under/Goin' Under.ssc", true },
		{ "StepMania 5/MechaTribe Assault/Mecha-Tribe Assault.ssc", true },
		{ "StepMania 5/Springtime/Springtime.ssc", true },
	};

	for( auto const &it : items )
	{
		Song song;
		bool ok;
		if( it.ssc ) { SSCLoader l; ok = l.LoadFromSimfile( EngineTestEnv::SongPath( it.path ), song, false ); }
		else         { SMLoader  l; ok = l.LoadFromSimfile( EngineTestEnv::SongPath( it.path ), song, false ); }

		std::printf( "\n=== %s  (loaded=%d)\n", it.path, ok ? 1 : 0 );
		std::printf( "    title=[%s] subtitle=[%s] artist=[%s] offset=%.6f bpm0=%.4f charts=%d\n",
			song.m_sMainTitle.c_str(), song.m_sSubTitle.c_str(), song.m_sArtist.c_str(),
			song.m_SongTiming.m_fBeat0OffsetInSeconds,
			song.m_SongTiming.GetBPMAtBeat( 0.0f ),
			(int)song.GetAllSteps().size() );

		for( Steps *s : song.GetAllSteps() )
		{
			NoteData nd;
			s->GetNoteData( nd );
			std::printf( "    { \"%s\", Difficulty_%s, %d, %d, %d },\n",
				s->m_StepsTypeStr.c_str(),
				DifficultyToString( s->GetDifficulty() ).c_str(),
				s->GetMeter(),
				nd.GetNumTracks(),
				nd.GetNumTapNotesNoTiming() );
		}
	}
	SUCCEED( "dump complete" );
}
