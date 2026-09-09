// Characterization tests for SongOptions -- the per-song modifier set
// (music rate, haste, autosync, assist clap/metronome, BG flags) and
// its "1.5xMusic, Clap" string codec. Unlike PlayerOptions,
// SongOptions::FromOneModString has no NOTESKIN / GAMESTATE dependency.
//
// Pins CURRENT behaviour, bug-for-bug (including: FromString silently
// ignores an unknown mod, and the xMusic token is matched
// case-insensitively by regex). See DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "SongOptions.h"
#include "GameConstantsAndTypes.h"

#include "catch_amalgamated.hpp"

using Catch::Approx;

TEST_CASE( "SongOptions::FromString parses the xMusic rate token", "[SongOptions]" )
{
	EngineTestEnv::Require();

	SongOptions so;
	so.FromString( "1.5xMusic" );
	CHECK( so.m_fMusicRate == Approx( 1.5f ) );

	so.Init();
	so.FromString( "2xMusic" ); // integer form
	CHECK( so.m_fMusicRate == Approx( 2.0f ) );

	so.Init();
	so.FromString( "0.75xmusic" ); // lower-case
	CHECK( so.m_fMusicRate == Approx( 0.75f ) );
}

TEST_CASE( "SongOptions::FromString parses the on/off keyword mods", "[SongOptions]" )
{
	EngineTestEnv::Require();

	SongOptions so;

	so.FromString( "clap" );
	CHECK( so.m_bAssistClap );
	so.FromString( "no clap" );
	CHECK_FALSE( so.m_bAssistClap );

	so.FromString( "metronome" );
	CHECK( so.m_bAssistMetronome );

	so.Init();
	so.FromString( "autosync" );
	CHECK( so.m_AutosyncType == AutosyncType_Song );
	so.FromString( "autosyncmachine" );
	CHECK( so.m_AutosyncType == AutosyncType_Machine );
	so.FromString( "no autosync" );
	CHECK( so.m_AutosyncType == AutosyncType_Off );

	so.Init();
	so.FromString( "haste" );
	CHECK( so.m_fHaste == Approx( 1.0f ) );
	so.FromString( "no haste" );
	CHECK( so.m_fHaste == Approx( 0.0f ) );
}

TEST_CASE( "SongOptions::FromString ignores an unknown mod and keeps the rest", "[SongOptions]" )
{
	EngineTestEnv::Require();

	SongOptions so;
	so.FromString( "clap, notamod, metronome" );
	CHECK( so.m_bAssistClap );
	CHECK( so.m_bAssistMetronome );
	// nothing threw, nothing else changed
	CHECK( so.m_fMusicRate == Approx( 1.0f ) );
}

TEST_CASE( "SongOptions::Init resets to the defaults", "[SongOptions]" )
{
	EngineTestEnv::Require();

	SongOptions so;
	so.FromString( "1.3xMusic, clap, metronome, autosyncmachine, haste" );
	so.Init();

	SongOptions fresh;
	CHECK( so == fresh );
	CHECK( so.m_fMusicRate == Approx( 1.0f ) );
	CHECK_FALSE( so.m_bAssistClap );
	CHECK( so.m_AutosyncType == AutosyncType_Off );
}

TEST_CASE( "SongOptions GetString / FromString round-trip via operator==", "[SongOptions]" )
{
	EngineTestEnv::Require();

	for( const char *mods : { "1.5xMusic",
	                          "Clap, Metronome",
	                          "1.25xMusic, Clap, AutosyncSong",
	                          "AutosyncMachine, Haste",
	                          "0.8xMusic, Metronome, StaticBG, RandomBG" } )
	{
		CAPTURE( mods );
		SongOptions a;
		a.FromString( mods );

		SongOptions b;
		b.FromString( a.GetString() );

		CHECK( b == a );
		CHECK( b.GetString() == a.GetString() );
	}
}
