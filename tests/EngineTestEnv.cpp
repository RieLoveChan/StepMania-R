#include "global.h"
#include "EngineTestEnv.h"

#include "RageUtil.h"       // SAFE_DELETE
#include "LuaManager.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "GameManager.h"

#include "catch_amalgamated.hpp"

// SM_TESTS_ARGV0 (absolute path to the sm_tests binary) and
// SM_TEST_DATA_DIR (absolute path to tests/data) come from this generated
// header -- see tests/CMakeLists.txt. It uses raw string literals so
// Windows backslashes in the paths are harmless.
#include "EngineTestEnvPaths.h"

#ifndef SM_TESTS_ARGV0
#error "EngineTestEnvPaths.h did not define SM_TESTS_ARGV0"
#endif
#ifndef SM_TEST_DATA_DIR
#error "EngineTestEnvPaths.h did not define SM_TEST_DATA_DIR"
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

		// GameManager's ctor is trivial -- it only registers GAMEMAN with
		// Lua; every game/style/StepsType table it serves is file-scope
		// static data in GameManager.cpp. It is here (not in the "not
		// provided" list) because it needs no init of its own and any
		// real simfile load resolves its #STEPSTYPE through
		// GAMEMAN->StringToStepsType. Needs LUA.
		if( GAMEMAN == nullptr )
			GAMEMAN = new GameManager;

		// The committed corpus, read-only, at a fixed vpath.
		FILEMAN->Mount( "dir", SM_TEST_DATA_DIR, "/testdata" );

		g_bUp = true;
	}

	void TearDown()
	{
		if( !g_bUp )
			return;
		// Reverse of construction order. SAFE_DELETE nulls each global,
		// so a later Require() in the same process would rebuild cleanly.
		SAFE_DELETE( GAMEMAN );
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
}
