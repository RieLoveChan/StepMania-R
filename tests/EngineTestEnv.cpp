#include "global.h"
#include "EngineTestEnv.h"

#include "RageUtil.h"       // SAFE_DELETE
#include "LuaManager.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "GameManager.h"

#include "catch_amalgamated.hpp"

// SM_TESTS_ARGV0 (sm_tests binary), SM_TEST_DATA_DIR (tests/data) and
// SM_SONGS_DIR (the repo's Songs/ tree) come from this generated header
// -- see tests/CMakeLists.txt. It uses raw string literals so Windows
// backslashes in the paths are harmless.
#include "EngineTestEnvPaths.h"

#ifndef SM_TESTS_ARGV0
#error "EngineTestEnvPaths.h did not define SM_TESTS_ARGV0"
#endif
#ifndef SM_TEST_DATA_DIR
#error "EngineTestEnvPaths.h did not define SM_TEST_DATA_DIR"
#endif
#ifndef SM_SONGS_DIR
#error "EngineTestEnvPaths.h did not define SM_SONGS_DIR"
#endif

namespace
{
	bool g_bUp = false;

	void BringUp()
	{
		if( g_bUp )
			return;

		// Construction order mirrors sm_main() and is load-bearing:
		//   * RageFileManager's ctor calls LUA->Get(), so LUA must exist.
		//   * RageLog's ctor opens a RageFile, whose Open() asserts
		//     FILEMAN != nullptr, so FILEMAN must exist.
		if( LUA == nullptr )
			LUA = new LuaManager;

		if( FILEMAN == nullptr )
		{
			// argv[0]: ignored on Windows (RageFileManager uses
			// GetModuleFileName), but on UNIX/macOS it must be a real
			// path so ChangeToDirOfExecutable can chdir rather than fall
			// into its LOG->Warn branch -- and LOG is still null here.
			FILEMAN = new RageFileManager( SM_TESTS_ARGV0 );
		}

		if( LOG == nullptr )
		{
			LOG = new RageLog;
			// Characterization runs must not touch the real Logs/ tree.
			// (RageLog's ctor already tried to open /Logs/timelog.txt and
			// failed quietly to stderr -- nothing is mounted writable at
			// "/"; that is expected and harmless.)
			LOG->SetShowLogOutput( false );
			LOG->SetLogToDisk( false );
			LOG->SetInfoToDisk( false );
			LOG->SetUserLogToDisk( false );
			LOG->SetFlushing( false );
		}

		// PrefsManager: registers a few hundred Preference<T> objects and
		// tries to read Data/{Defaults,Preferences,Static}.ini -- none of
		// which are mounted here, so every preference keeps its compiled
		// default (deterministic, and independent of the dev's local
		// Preferences.ini). Needed by the dir-only loaders (DWILoader
		// reads m_bQuirksMode, courses read m_bFastLoad, ...). The ctor
		// touches LUA (global registration) and FILEMAN (the .ini reads,
		// which no-op on miss); the dtor only unregisters from LUA, so
		// LUA must outlive it -- the reverse-order teardown handles that.
		if( PREFSMAN == nullptr )
			PREFSMAN = new PrefsManager;

		// GameManager's ctor is trivial -- it only registers GAMEMAN with
		// Lua; every game/style/StepsType table it serves is file-scope
		// static data in GameManager.cpp. It is here (not in the "not
		// provided" list) because it needs no init of its own and any
		// real simfile load resolves its #STEPSTYPE through
		// GAMEMAN->StringToStepsType. Needs LUA.
		if( GAMEMAN == nullptr )
			GAMEMAN = new GameManager;

		// tests/data/ -> /testdata (non-simfile fixtures); the repo's
		// Songs/ tree -> /Songs (the real committed simfiles the
		// parse-regression loads). Both read-only "dir" mounts; lazy, so
		// mounting Songs/ costs nothing until a path under it is read.
		FILEMAN->Mount( "dir", SM_TEST_DATA_DIR, "/testdata" );
		FILEMAN->Mount( "dir", SM_SONGS_DIR, "/Songs" );

		g_bUp = true;
	}

	void TearDown()
	{
		if( !g_bUp )
			return;
		// Reverse of construction order. SAFE_DELETE nulls each global,
		// so a later Require() in the same process would rebuild cleanly.
		// PREFSMAN's dtor calls LUA->UnsetGlobal, so it must go before LUA.
		SAFE_DELETE( GAMEMAN );
		SAFE_DELETE( PREFSMAN );
		SAFE_DELETE( LOG );
		SAFE_DELETE( FILEMAN );
		SAFE_DELETE( LUA );
		g_bUp = false;
	}

	// Tears the environment down once, after the last test case. Catch2
	// owns no engine state, so this is the only cleanup hook needed.
	struct EngineTestEnvListener : Catch::EventListenerBase
	{
		using Catch::EventListenerBase::EventListenerBase;
		void testRunEnded( Catch::TestRunStats const & ) override { TearDown(); }
	};
}

CATCH_REGISTER_LISTENER( EngineTestEnvListener )

namespace EngineTestEnv
{
	void Require() { BringUp(); }

	RString TestDataPath( const RString &sRelative )
	{
		return RString( "/testdata/" ) + sRelative;
	}

	RString SongPath( const RString &sRelative )
	{
		return RString( "/Songs/" ) + sRelative;
	}
}
