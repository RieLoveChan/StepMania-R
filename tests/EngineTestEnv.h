// EngineTestEnv -- bring up the minimum set of engine singletons a
// characterization test needs in order to reach code that logs, reads
// files, or touches Lua. This is the shared bootstrap fixture ADR 0006
// phases 3-4 and the committed simfile corpus were blocked on.
//
// What it constructs (process-global, once per sm_tests run):
//
//   LUA      (LuaManager)      -- a lua_State + RegisterTypes(); required
//                                 because RageFileManager's ctor calls
//                                 LUA->Get().
//   FILEMAN  (RageFileManager) -- RageFile I/O. Mounts the committed
//                                 tests/data/ tree at "/testdata".
//   LOG      (RageLog)         -- so the LOG->UserLog / LOG->Warn error
//                                 branches in the parsers actually run
//                                 instead of dereferencing a null LOG.
//                                 Disk output is turned off.
//   PREFSMAN (PrefsManager)    -- read by the dir-only loaders (DWILoader
//                                 -> m_bQuirksMode, courses -> m_bFastLoad)
//                                 and various Song/Steps paths. No .ini is
//                                 mounted, so every preference keeps its
//                                 compiled default (deterministic).
//   GAMEMAN  (GameManager)     -- ctor is trivial (Lua registration only;
//                                 all game/style/StepsType tables are
//                                 file-scope static data), and any real
//                                 simfile load resolves #STEPSTYPE
//                                 through GAMEMAN->StringToStepsType.
//   MESSAGEMAN (MessageManager) -- trivial ctor; also stops LuaHelpers'
//                                 script-error path from dereferencing a
//                                 null MESSAGEMAN.
//   GAMESTATE (GameState)      -- constructed but NOT Reset() (its ctor
//                                 skips that on purpose), so the pointer
//                                 is valid but GAMESTATE->GetCurrentGame()
//                                 and friends are still not safe to call.
//   THEME    (ThemeManager)    -- CONSTRUCTED ONLY. No theme is switched
//                                 in: SwitchThemeAndLanguage() runs the
//                                 theme's Lua and SIGSEGVs headlessly.
//                                 So the pointer is valid, but every
//                                 ThemeMetric<T> / CommonMetrics::* stays
//                                 unset -- reading a metric value still
//                                 asserts. (Backlog item 17.)
//   NOTESKIN (NoteSkinManager) -- trivial ctor; non-null unblocks the
//                                 PlayerOptions mod-processing ASSERT and
//                                 NOTESKIN->... calls.
//   SONGMAN  (SongManager)     -- constructed; InitAll() (the slow song
//                                 scan) is NOT called, so it holds no
//                                 songs/courses until a test adds them.
//
// What it deliberately does NOT do: switch a theme, populate SONGMAN,
// bring up the renderer or the audio device. Anything that needs those
// stays --SelfTest smoke-test territory (AGENTS.md, ADR 0006).
// Song::LoadFromSongDir (the full song-directory load, with cache) still
// needs more than this.
//
// Usage: call EngineTestEnv::Require() at the top of any TEST_CASE that
// needs the above. It is idempotent -- the first call constructs, later
// calls are no-ops -- and a Catch2 listener tears the singletons down
// once after the whole run. Tests that never call Require() pay nothing.

#ifndef TESTS_ENGINE_TEST_ENV_H
#define TESTS_ENGINE_TEST_ENV_H

#include "global.h"

namespace EngineTestEnv
{
	// Idempotent. On first call constructs, in this order:
	//   LUA -> ActorUtil::InitFileTypeLists() -> FILEMAN -> LOG
	//       -> PREFSMAN -> (mount Themes/, NoteSkins/) -> MESSAGEMAN
	//       -> GAMESTATE -> GAMEMAN -> THEME (ctor only) -> NOTESKIN
	//       -> SONGMAN
	// The order is load-bearing: RageFileManager's ctor calls LUA->Get();
	// RageLog's ctor opens a RageFile, which asserts FILEMAN != null;
	// PrefsManager's ctor reads .ini files via FILEMAN and registers with
	// LUA. InitFileTypeLists() populates the static extension<->filetype
	// maps (needed by the sound/image readers). MESSAGEMAN precedes
	// GAMESTATE (matches sm_main). Teardown is the reverse -- every
	// manager whose dtor calls LUA->UnsetGlobal is destroyed before LUA;
	// the filetype maps are static and left as-is.
	void Require();

	// Turn a path relative to tests/data/ into the vpath it is mounted
	// at -- TestDataPath("foo.txt") == "/testdata/foo.txt". Calling this
	// does not itself require the environment to be up, but reading the
	// path obviously does.
	RString TestDataPath( const RString &sRelative );

	// Same, for the repo's Songs/ tree -- SongPath("StepMania 5/Springtime/Springtime.ssc")
	// == "/Songs/StepMania 5/Springtime/Springtime.ssc". The parse-
	// regression loads the real committed simfiles from here; hand-authored
	// toy simfiles are deliberately not used (tests/data/README.md).
	RString SongPath( const RString &sRelative );
}

#endif
