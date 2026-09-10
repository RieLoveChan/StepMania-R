// Parse-regression for .sma via SMALoader::LoadFromDir (ADR 0006 phase 4).
//
// There is no real .sma simfile anywhere -- after an extended search the
// maintainer could not find a single one (the format is an extinct
// 2009-2011 SMA-editor variant by Aldo Fregoso / Jason Felds). Per the
// 2026-09-10 decision, the CURRENT SMALoader read behavior is taken as
// correct, and this test pins it so a future refactor of the shared
// SMLoader base cannot silently change how .sma parses.
//
// The fixture (tests/data/sma-fixture/fixture.sma) is SYNTHETIC -- entirely
// hand-authored, no upstream source to diff against. It deliberately
// exercises what SMALoader adds on top of SMLoader:
//
//   #SMAVERSION       -- ignored
//   #ROWSPERBEAT:0=4  -- sets the row<->beat divisor. A #BPMS/#STOPS value
//                        with a trailing 'r' ("8r") is a ROW and gets
//                        divided by 4; a bare value ("24") is a BEAT and
//                        is taken literally. This test pins both.
//   #BEATSPERMEASURE  -- -> TimeSignatureSegments. The first given segment
//                        is at beat 4, not 0, so a 4/4 is back-filled at
//                        row 0 ahead of it.
//   #SPEED            -- -> SpeedSegments. The third field "32s" (trailing
//                        's') selects SpeedSegment::UNIT_SECONDS; without
//                        it the unit is UNIT_BEATS.
//   #MULTIPLIER       -- -> ComboSegments. A "row=combos" pair (2 values)
//                        makes missCombo == combo; "row=combos=misses"
//                        (3 values) sets them independently.
//
// One quirk this pins, bug-for-bug: tags that appear BEFORE the single
// #NOTES block land on song.m_SongTiming (offset, time-sig, speed, combo,
// delay); #BPMS/#STOPS only fill a local buffer that ProcessBPMsAndStops
// flushes when #NOTES is reached -- and by then the target is the Steps'
// timing, so the song-level BPM/stop lists come out EMPTY and the Steps
// timing carries them. If a refactor moves a number here, stop and decide
// whether the change is intended; do not just edit the expectation.
// Run `sm_tests "[smadump]"` to re-baseline. See
// DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "NotesLoaderSMA.h"
#include "Song.h"
#include "Steps.h"
#include "NoteData.h"
#include "TimingData.h"
#include "TimingSegments.h"
#include "GameManager.h"
#include "Difficulty.h"

#include "catch_amalgamated.hpp"

#include <cstdio>
#include <vector>

using Catch::Approx;

namespace
{
	// tests/data/sma-fixture/ -> /testdata/sma-fixture/ .
	const char *kFixtureDir = "sma-fixture/";

	template <typename T>
	T *SegAt( const TimingData &td, TimingSegmentType t, size_t i )
	{
		const std::vector<TimingSegment *> &segs = td.GetTimingSegments( t );
		REQUIRE( segs.size() > i );
		return static_cast<T *>( segs[i] );
	}
}

TEST_CASE( "SMALoader::LoadFromDir parses the synthetic .sma fixture",
           "[NotesLoader][SMALoader][sma][regression]" )
{
	EngineTestEnv::Require();

	const RString dir = EngineTestEnv::TestDataPath( kFixtureDir );

	std::vector<RString> applicable;
	SMALoader loader;
	loader.GetApplicableFiles( dir, applicable );
	REQUIRE( applicable.size() == 1 );

	Song song;
	REQUIRE( loader.LoadFromDir( dir, song ) );

	// --- metadata (plain SMLoader tag handling, shared with .sm) ---
	CHECK( song.m_sMainTitle == "SMA Fixture (Synthetic)" );
	CHECK( song.m_sSubTitle == "pinned" );
	CHECK( song.m_sArtist == "Test Fixture" );
	CHECK( song.m_sGenre == "test" );
	CHECK( song.m_sMusicFile == "fixture.ogg" );
	CHECK( song.m_fMusicSampleStartSeconds == Approx( 12.5f ) );
	CHECK( song.m_fMusicSampleLengthSeconds == Approx( 8.0f ) );

	const TimingData &songTiming = song.m_SongTiming;

	// --- #OFFSET (pre-#NOTES -> song timing) ---
	CHECK( songTiming.m_fBeat0OffsetInSeconds == Approx( -0.007f ) );

	// --- #BPMS / #STOPS never reach song timing (see file header) ---
	CHECK( songTiming.GetTimingSegments( SEGMENT_BPM ).empty() );
	CHECK( songTiming.GetTimingSegments( SEGMENT_STOP ).empty() );

	// --- #DELAYS:24=0.250 -- bare value => beat 24 => row 24*48 = 1152 ---
	{
		const std::vector<TimingSegment *> &d = songTiming.GetTimingSegments( SEGMENT_DELAY );
		REQUIRE( d.size() == 1 );
		CHECK( d[0]->GetRow() == 1152 );
		CHECK( SegAt<DelaySegment>( songTiming, SEGMENT_DELAY, 0 )->GetPause() == Approx( 0.25f ) );
	}

	// --- #BEATSPERMEASURE:4=3,8=5 -> 4/4 back-filled at row 0, then
	//     3/4 at beat 4 (row 192), 5/4 at beat 8 (row 384) ---
	{
		const std::vector<TimingSegment *> &ts = songTiming.GetTimingSegments( SEGMENT_TIME_SIG );
		REQUIRE( ts.size() == 3 );
		CHECK( ts[0]->GetRow() == 0 );
		CHECK( SegAt<TimeSignatureSegment>( songTiming, SEGMENT_TIME_SIG, 0 )->GetNum() == 4 );
		CHECK( ts[1]->GetRow() == 192 );
		CHECK( SegAt<TimeSignatureSegment>( songTiming, SEGMENT_TIME_SIG, 1 )->GetNum() == 3 );
		CHECK( ts[2]->GetRow() == 384 );
		CHECK( SegAt<TimeSignatureSegment>( songTiming, SEGMENT_TIME_SIG, 2 )->GetNum() == 5 );
	}

	// --- #SPEED:0=1=0,4=2=1,8=0.5=32s -> 3 SpeedSegments;
	//     first two UNIT_BEATS, the "32s" one UNIT_SECONDS ---
	{
		const std::vector<TimingSegment *> &sp = songTiming.GetTimingSegments( SEGMENT_SPEED );
		REQUIRE( sp.size() == 3 );
		CHECK( sp[0]->GetRow() == 0 );
		CHECK( SegAt<SpeedSegment>( songTiming, SEGMENT_SPEED, 0 )->GetRatio() == Approx( 1.0f ) );
		CHECK( SegAt<SpeedSegment>( songTiming, SEGMENT_SPEED, 0 )->GetUnit() == SpeedSegment::UNIT_BEATS );
		CHECK( sp[1]->GetRow() == 192 );
		CHECK( SegAt<SpeedSegment>( songTiming, SEGMENT_SPEED, 1 )->GetRatio() == Approx( 2.0f ) );
		CHECK( SegAt<SpeedSegment>( songTiming, SEGMENT_SPEED, 1 )->GetDelay() == Approx( 1.0f ) );
		CHECK( sp[2]->GetRow() == 384 );
		CHECK( SegAt<SpeedSegment>( songTiming, SEGMENT_SPEED, 2 )->GetRatio() == Approx( 0.5f ) );
		CHECK( SegAt<SpeedSegment>( songTiming, SEGMENT_SPEED, 2 )->GetDelay() == Approx( 32.0f ) );
		CHECK( SegAt<SpeedSegment>( songTiming, SEGMENT_SPEED, 2 )->GetUnit() == SpeedSegment::UNIT_SECONDS );
	}

	// --- #MULTIPLIER:0=2,4=3=7 -> 2 ComboSegments;
	//     "0=2" (2 fields) => miss == combo; "4=3=7" (3 fields) => miss 7 ---
	{
		const std::vector<TimingSegment *> &co = songTiming.GetTimingSegments( SEGMENT_COMBO );
		REQUIRE( co.size() == 2 );
		CHECK( co[0]->GetRow() == 0 );
		CHECK( SegAt<ComboSegment>( songTiming, SEGMENT_COMBO, 0 )->GetCombo() == 2 );
		CHECK( SegAt<ComboSegment>( songTiming, SEGMENT_COMBO, 0 )->GetMissCombo() == 2 );
		CHECK( co[1]->GetRow() == 192 );
		CHECK( SegAt<ComboSegment>( songTiming, SEGMENT_COMBO, 1 )->GetCombo() == 3 );
		CHECK( SegAt<ComboSegment>( songTiming, SEGMENT_COMBO, 1 )->GetMissCombo() == 7 );
	}

	// --- the one #NOTES block ---
	auto const &steps = song.GetAllSteps();
	REQUIRE( steps.size() == 1 );
	Steps *s = steps[0];
	CHECK( s->m_StepsTypeStr == "dance-single" );
	CHECK( s->m_StepsType == GAMEMAN->StringToStepsType( "dance-single" ) );
	CHECK( s->GetDifficulty() == Difficulty_Challenge );
	CHECK( s->GetMeter() == 9 );

	NoteData nd;
	s->GetNoteData( nd );
	CHECK( nd.GetNumTracks() == 4 );
	CHECK( nd.GetNumTapNotesNoTiming() == 4 );

	// --- #BPMS / #STOPS end up on the Steps' timing, with #ROWSPERBEAT
	//     row-translation applied to the 'r'-suffixed values:
	//       #BPMS:0=120,8r=150   -> beat 0 = 120, row 8/4 = beat 2 (row 96) = 150
	//       #STOPS:16r=0.5       -> row 16/4 = beat 4 (row 192), 0.5 s ---
	{
		const TimingData &st = s->m_Timing;
		const std::vector<TimingSegment *> &bpm = st.GetTimingSegments( SEGMENT_BPM );
		REQUIRE( bpm.size() == 2 );
		CHECK( bpm[0]->GetRow() == 0 );
		CHECK( SegAt<BPMSegment>( st, SEGMENT_BPM, 0 )->GetBPM() == Approx( 120.0f ) );
		CHECK( bpm[1]->GetRow() == 96 );
		CHECK( SegAt<BPMSegment>( st, SEGMENT_BPM, 1 )->GetBPM() == Approx( 150.0f ) );

		const std::vector<TimingSegment *> &stop = st.GetTimingSegments( SEGMENT_STOP );
		REQUIRE( stop.size() == 1 );
		CHECK( stop[0]->GetRow() == 192 );
		CHECK( SegAt<StopSegment>( st, SEGMENT_STOP, 0 )->GetPause() == Approx( 0.5f ) );
	}
}

// Hidden. Run with:  sm_tests "[smadump]"
TEST_CASE( "dump SMA fixture loader values", "[.][sma][smadump]" )
{
	EngineTestEnv::Require();

	const RString dir = EngineTestEnv::TestDataPath( kFixtureDir );
	std::vector<RString> applicable;
	SMALoader loader;
	loader.GetApplicableFiles( dir, applicable );

	Song song;
	bool ok = !applicable.empty() && loader.LoadFromDir( dir, song );

	std::printf( "\n=== %s (applicable=%d loaded=%d)\n",
		kFixtureDir, (int)applicable.size(), ok ? 1 : 0 );
	std::printf( "    title=[%s] subtitle=[%s] artist=[%s] genre=[%s] music=[%s]\n",
		song.m_sMainTitle.c_str(), song.m_sSubTitle.c_str(),
		song.m_sArtist.c_str(), song.m_sGenre.c_str(), song.m_sMusicFile.c_str() );
	std::printf( "    offset=%.5f sampleStart=%.4f sampleLen=%.4f\n",
		song.m_SongTiming.m_fBeat0OffsetInSeconds,
		song.m_fMusicSampleStartSeconds, song.m_fMusicSampleLengthSeconds );

	auto dumpSegs = [&]( const char *tag, const TimingData &td, const char *name, TimingSegmentType t ) {
		const std::vector<TimingSegment *> &segs = td.GetTimingSegments( t );
		std::printf( "    [%s] %s: %d segment(s):", tag, name, (int)segs.size() );
		for( TimingSegment *seg : segs )
		{
			std::printf( " {row=%d", seg->GetRow() );
			switch( t )
			{
			case SEGMENT_BPM:
				std::printf( " bpm=%.4f", static_cast<BPMSegment *>( seg )->GetBPM() ); break;
			case SEGMENT_STOP:
				std::printf( " s=%.4f", static_cast<StopSegment *>( seg )->GetPause() ); break;
			case SEGMENT_DELAY:
				std::printf( " s=%.4f", static_cast<DelaySegment *>( seg )->GetPause() ); break;
			case SEGMENT_TIME_SIG:
				std::printf( " %d/%d", static_cast<TimeSignatureSegment *>( seg )->GetNum(),
					static_cast<TimeSignatureSegment *>( seg )->GetDen() ); break;
			case SEGMENT_SPEED:
				std::printf( " ratio=%.4f delay=%.4f unit=%d",
					static_cast<SpeedSegment *>( seg )->GetRatio(),
					static_cast<SpeedSegment *>( seg )->GetDelay(),
					(int)static_cast<SpeedSegment *>( seg )->GetUnit() ); break;
			case SEGMENT_COMBO:
				std::printf( " combo=%d miss=%d", static_cast<ComboSegment *>( seg )->GetCombo(),
					static_cast<ComboSegment *>( seg )->GetMissCombo() ); break;
			case SEGMENT_TICKCOUNT:
				std::printf( " ticks=%d", static_cast<TickcountSegment *>( seg )->GetTicks() ); break;
			default: break;
			}
			std::printf( "}" );
		}
		std::printf( "\n" );
	};
	for( const char *which : { "song", "steps0" } )
	{
		const bool isSong = ( which[0] == 's' && which[1] == 'o' );
		if( !isSong && song.GetAllSteps().empty() )
			break;
		const TimingData &td = isSong ? song.m_SongTiming : song.GetAllSteps()[0]->m_Timing;
		dumpSegs( which, td, "BPM",      SEGMENT_BPM );
		dumpSegs( which, td, "STOP",     SEGMENT_STOP );
		dumpSegs( which, td, "DELAY",    SEGMENT_DELAY );
		dumpSegs( which, td, "TIME_SIG", SEGMENT_TIME_SIG );
		dumpSegs( which, td, "SPEED",    SEGMENT_SPEED );
		dumpSegs( which, td, "COMBO",    SEGMENT_COMBO );
		dumpSegs( which, td, "TICKCOUNT",SEGMENT_TICKCOUNT );
	}

	for( Steps *s : song.GetAllSteps() )
	{
		NoteData nd;
		s->GetNoteData( nd );
		std::printf( "    chart { \"%s\", Difficulty_%s, meter=%d, tracks=%d, taps=%d }\n",
			s->m_StepsTypeStr.c_str(),
			DifficultyToString( s->GetDifficulty() ).c_str(),
			s->GetMeter(), nd.GetNumTracks(), nd.GetNumTapNotesNoTiming() );
	}
	SUCCEED( "dump complete" );
}
