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
//   GAMESTATE (GameState)      -- constructed, and given a current game
//                                 (GAMEMAN->GetDefaultGame()) but NOT
//                                 Reset(). The theme load needs a current
//                                 game; most other GAMESTATE-> paths are
//                                 still not safe to call.
//   NOTESKIN (NoteSkinManager) -- trivial ctor; non-null unblocks the
//                                 PlayerOptions mod-processing ASSERT.
//                                 Built before GAMEMAN->GetDefaultGame().
//   THEME    (ThemeManager)    -- switched to "SMRTest", the scripts-free
//                                 minimal theme in tests/data/test-theme/.
//                                 A theme MUST be loaded: engine ctors
//                                 (Song, SongManager, ...) read
//                                 ThemeMetric<T> via GetValue(), which
//                                 ASSERT_M( IsSet() ) -- fatal, and on
//                                 Unix sm_crash()s the binary. A real
//                                 theme can't load headlessly (its metric
//                                 values are Lua that touches a live
//                                 engine); SMRTest sets FallbackTheme= and
//                                 has no Scripts/, so undefined metrics
//                                 resolve "missing -> ignore -> nil".
//   SONGMAN  (SongManager)     -- constructed (its ctor's ThemeMetric
//                                 reads resolve against SMRTest now).
//                                 InitAll() (the slow song scan) is NOT
//                                 called -- it holds no songs/courses.
//
// What it deliberately does NOT do: call GAMESTATE->Reset(), populate
// SONGMAN, bring up the renderer or the audio device. Anything that
// needs those stays --SelfTest smoke-test territory (AGENTS.md,
// ADR 0006). Song::LoadFromSongDir (the full song-directory load, with
// cache) still needs more than this.
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
	//       -> PREFSMAN -> (mount /testdata /Songs /Themes /NoteSkins)
	//       -> MESSAGEMAN -> GAMESTATE -> GAMEMAN -> NOTESKIN
	//       -> GAMESTATE->SetCurGame(default) -> THEME + switch to SMRTest
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
