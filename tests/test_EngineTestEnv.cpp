// Characterization / contract test for the EngineTestEnv fixture itself.
// Guards the bring-up: which singletons Require() constructs, that
// PrefsManager comes up with its compiled defaults (no .ini is mounted,
// so the values must not depend on the developer's Preferences.ini), and
// the deliberate limits (THEME is constructed but no theme is loaded).
//
// See DocsAgents/adr/0006-test-harness.md "Phase 3-4 enabler".

#include "global.h"
#include "EngineTestEnv.h"

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

#include "catch_amalgamated.hpp"

using Catch::Approx;

TEST_CASE( "EngineTestEnv::Require brings up the engine singletons", "[EngineTestEnv]" )
{
	EngineTestEnv::Require();

	CHECK( LUA != nullptr );
	CHECK( FILEMAN != nullptr );
	CHECK( LOG != nullptr );
	CHECK( PREFSMAN != nullptr );
	CHECK( GAMEMAN != nullptr );
	CHECK( MESSAGEMAN != nullptr );
	CHECK( GAMESTATE != nullptr );
	CHECK( THEME != nullptr );
	CHECK( NOTESKIN != nullptr );
	CHECK( SONGMAN != nullptr );
}

TEST_CASE( "EngineTestEnv leaves THEME constructed but with no theme loaded", "[EngineTestEnv][theme]" )
{
	EngineTestEnv::Require();

	// SwitchThemeAndLanguage() is intentionally not called (it runs the
	// theme's Lua and crashes headlessly -- backlog item 17), so no theme
	// is loaded and metric reads are no-ops rather than real values.
	REQUIRE( THEME != nullptr );
	CHECK_FALSE( THEME->IsThemeLoaded() );
	CHECK( THEME->GetCurThemeName().empty() );
}

TEST_CASE( "EngineTestEnv::Require is idempotent", "[EngineTestEnv]" )
{
	EngineTestEnv::Require();
	void *lua = LUA, *fileman = FILEMAN, *log = LOG, *prefs = PREFSMAN, *gameman = GAMEMAN;
	void *msg = MESSAGEMAN, *gs = GAMESTATE, *theme = THEME, *ns = NOTESKIN, *sm = SONGMAN;

	EngineTestEnv::Require(); // second call must not rebuild anything

	CHECK( (void *)LUA == lua );
	CHECK( (void *)FILEMAN == fileman );
	CHECK( (void *)LOG == log );
	CHECK( (void *)PREFSMAN == prefs );
	CHECK( (void *)GAMEMAN == gameman );
	CHECK( (void *)MESSAGEMAN == msg );
	CHECK( (void *)GAMESTATE == gs );
	CHECK( (void *)THEME == theme );
	CHECK( (void *)NOTESKIN == ns );
	CHECK( (void *)SONGMAN == sm );
}

TEST_CASE( "PrefsManager comes up with compiled defaults (no .ini mounted)", "[EngineTestEnv][prefs]" )
{
	EngineTestEnv::Require();

	// Defaults straight from PrefsManager's ctor initializer list. These
	// are what the dir-only loaders will see: DWILoader reads
	// m_bQuirksMode, the course loaders read m_bFastLoad.
	CHECK( PREFSMAN->m_bQuirksMode.Get() == false );
	CHECK( PREFSMAN->m_bFastLoad.Get() == true );
	CHECK( PREFSMAN->m_fGlobalOffsetSeconds.Get() == Approx( -0.008f ) );
}
