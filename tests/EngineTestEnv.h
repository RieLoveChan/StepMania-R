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
//
// What it deliberately does NOT construct: GAMESTATE, THEME, SONGMAN, the
// renderer, the audio device. Anything that needs those stays --SelfTest
// smoke-test territory (AGENTS.md, ADR 0006). Song::LoadFromSongDir (the
// full song-directory load, with cache) still needs more than this.
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
	//   LUA -> FILEMAN -> LOG -> PREFSMAN -> GAMEMAN
	// The order is load-bearing: RageFileManager's ctor calls LUA->Get();
	// RageLog's ctor opens a RageFile, which asserts FILEMAN != null;
	// PrefsManager's ctor reads .ini files via FILEMAN and registers with
	// LUA. Teardown is the reverse (PREFSMAN's dtor calls LUA->UnsetGlobal).
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
