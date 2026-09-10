#include "global.h"
#include "EngineTestEnv.h"

#include "RageUtil.h"       // SAFE_DELETE
#include "LuaManager.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "MessageManager.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "NoteSkinManager.h"
#include "SongManager.h"
#include "ActorUtil.h"      // InitFileTypeLists

#include "catch_amalgamated.hpp"

// SM_TESTS_ARGV0 (sm_tests binary), SM_TEST_DATA_DIR (tests/data),
// SM_SONGS_DIR (the repo's Songs/ tree), SM_THEMES_DIR and
// SM_NOTESKINS_DIR come from this generated header -- see
// tests/CMakeLists.txt. It uses raw string literals so Windows
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
#ifndef SM_THEMES_DIR
#error "EngineTestEnvPaths.h did not define SM_THEMES_DIR"
#endif
#ifndef SM_NOTESKINS_DIR
#error "EngineTestEnvPaths.h did not define SM_NOTESKINS_DIR"
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

		// Populates the static extension<->filetype maps. Manager-free
		// (sm_main() calls it here, before FILEMAN); needed by the
		// file-type-aware readers -- RageSoundReader_FileReader::OpenFile
		// consults GetTypeExtensionList(FT_Sound). NOT idempotent (its
		// reverse-map build push_back()s), but BringUp() runs once.
		ActorUtil::InitFileTypeLists();

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

		// Read-only trees the managers below need. Relative paths like
		// SpecialFiles::THEMES_DIR ("Themes/") resolve against the VFS
		// root, so mounting the repo's Themes/ at /Themes makes
		// "Themes/*" listings work without MountInitialFilesystems().
		FILEMAN->Mount( "dir", SM_TEST_DATA_DIR, "/testdata" );
		FILEMAN->Mount( "dir", SM_SONGS_DIR, "/Songs" );
		FILEMAN->Mount( "dir", SM_THEMES_DIR, "/Themes" );
		FILEMAN->Mount( "dir", SM_NOTESKINS_DIR, "/NoteSkins" );

		// Messaging system. sm_main() brings this up right after PREFSMAN
		// and before GAMESTATE. Trivial ctor (Lua registration only); the
		// dtor unregisters from LUA, so LUA must outlive it. Having it up
		// also stops LuaHelpers' error-reporting path from dereferencing
		// a null MESSAGEMAN when a script fails to compile.
		if( MESSAGEMAN == nullptr )
			MESSAGEMAN = new MessageManager;

		// GameState. sm_main() creates it before GAMEMAN/THEME, and its
		// ctor deliberately does NOT call Reset() ("let the first screen
		// do it, so we can use PREFSMAN and THEME") -- so it needs only
		// LUA (registration) + BroadcastOnChange members that just store
		// a Message id (no broadcast at construction). Lots of GAMESTATE->
		// paths in real code become reachable with this up. Not Reset(),
		// so GAMESTATE->GetCurrentGame() etc. are still not safe to call.
		if( GAMESTATE == nullptr )
			GAMESTATE = new GameState;

		// GameManager's ctor is trivial -- it only registers GAMEMAN with
		// Lua; every game/style/StepsType table it serves is file-scope
		// static data in GameManager.cpp. Any real simfile load resolves
		// its #STEPSTYPE through GAMEMAN->StringToStepsType. Needs LUA.
		if( GAMEMAN == nullptr )
			GAMEMAN = new GameManager;

		// ThemeManager -- CONSTRUCTED ONLY, no theme switched in.
		//
		// SwitchThemeAndLanguage() is what would make THEME->GetMetric,
		// CommonMetrics::*, LocalizedString and ThemeMetric<T> return real
		// values -- but it also runs the whole theme's Lua
		// (Themes/{_fallback,default}/Scripts/*.lua) and refreshes the
		// screen-dimension metric cache, and that path SIGSEGVs in this
		// headless harness (theme code assumes a live engine: renderer,
		// SCREENMAN, sound). That is the same reason the original fixture
		// left THEME out entirely.
		//
		// So: THEME is non-null (unblocks code that only needs the
		// pointer, e.g. NOTESKIN paths and the PlayerOptions ASSERT), but
		// ThemeMetric::Read() stays a no-op (it gates on
		// THEME->IsThemeLoaded()), so any test that reads an actual
		// metric value will still hit the "m_Value.IsSet()" assert.
		// Loading metrics headlessly needs a separate, smaller mechanism
		// (a scripts-free minimal test theme, or a stub metric provider)
		// -- tracked in DocsAgents/modernization-backlog.md item 17.
		if( THEME == nullptr )
			THEME = new ThemeManager;

		// NoteSkinManager. Trivial ctor (Lua registration + invalid
		// members); it scans NoteSkins/<game>/ only when
		// RefreshNoteSkinData(pGame) is called. Needed non-null by
		// PlayerOptions mod processing (ASSERT) and NOTESKIN->... calls.
		if( NOTESKIN == nullptr )
			NOTESKIN = new NoteSkinManager;

		// SongManager ctor registers with LUA and Load()s a handful of
		// SongManager ThemeMetrics (group colours). Those Read()s no-op
		// while no theme is loaded, so the ctor is safe here. InitAll()
		// (the slow disk scan of every song folder) is deliberately NOT
		// called; tests that need populated songs/courses must arrange
		// that themselves.
		if( SONGMAN == nullptr )
			SONGMAN = new SongManager;

		g_bUp = true;
	}

	void TearDown()
	{
		if( !g_bUp )
			return;
		// Reverse of construction order. SAFE_DELETE nulls each global,
		// so a later Require() in the same process would rebuild cleanly.
		// Every manager whose dtor calls LUA->UnsetGlobal (PREFSMAN,
		// MESSAGEMAN, GAMESTATE, GAMEMAN, THEME, NOTESKIN, SONGMAN) must
		// be torn down before LUA.
		SAFE_DELETE( SONGMAN );
		SAFE_DELETE( NOTESKIN );
		SAFE_DELETE( THEME );
		SAFE_DELETE( GAMEMAN );
		SAFE_DELETE( GAMESTATE );
		SAFE_DELETE( MESSAGEMAN );
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
