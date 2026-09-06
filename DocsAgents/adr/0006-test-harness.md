---
type: Architecture Decision
title: Test harness — Catch2 v3 + engine OBJECT library
description: Pick Catch2 v3 (amalgamated) as the unit-test framework; split src/ into an OBJECT library so the exe and a new tests/ target share one build of the engine.
tags: [adr, testing, catch2, cmake, safety-net, modularization]
---

# Status

**Accepted** — 2026-09-03. Maintainer decision (framework choice +
OBJECT-library approach confirmed in-session). Scaffolding built on
`feature/test-harness`, gated on a green Windows Release build + `ctest`
pass (`AGENTS.md` §4).

**Merged to `5_1-new` — 2026-09-04.** All 8 CI jobs green (Windows/macOS/
Linux × plain build + `sm_tests`, plus the Lua.xml validator); Windows
Release build + `--SelfTest` also verified locally. Fixed en route:
`LoadingWindowGtk` OBJECT → STATIC (an OBJECT library's objects don't
propagate through *another* OBJECT library — broke the Linux `sm_tests`
link only; see `log.md` 2026-09-04). Phase 1 (Scaffold) is done; phases
2–4 below are open.

Supersedes the "Catch2 **or** doctest — TBD" note in
[`modernization-backlog.md`](../modernization-backlog.md) item 17.

# Context

`AGENTS.md`'s north star is **de-hard-coding the engine**, and the stated
first big objective below it is a **solid base to refactor from**. You
cannot safely pull `StepsType` out of an enum, split a 6,600-line
`ScreenEdit.cpp`, or migrate `RString` if nothing pins current behaviour.

Where we are ([`baseline.md`](../baseline.md)):

- **Headless smoke: DONE** (`f7249f3a95`). `--SelfTest` runs full engine
  init and exits 0. Proves "it boots", nothing finer.
- **Unit coverage: NONE.** `src/tests/` is 2004-era, Unix/Apple-only,
  needs ~30 MB of uncommitted data, `#error`s without SSE, `#if 0`
  throughout. Not wireable — it is a rewrite.
- `src/` builds **straight to an executable** (`add_executable` in
  `src/CMakeLists.txt:63,84`). There is no library another target could
  link, so there is nowhere to hang tests.

Two decisions were needed: the framework, and how a test binary gets at
the engine's code.

# Decision

## 1. Framework: Catch2 v3 (amalgamated distribution)

Vendored at `extern/Catch2/` — `catch_amalgamated.hpp` +
`catch_amalgamated.cpp` + `LICENSE.txt`, pinned to **v3.16.0**. Two files,
no submodule, built as one TU into the test target. BSL-1.0 (compatible;
StepMania is MIT-family).

Rationale over the alternatives (doctest, GoogleTest):

| Factor | Why it decided |
|---|---|
| Corpus regression | `GENERATE(from_range(...))` makes "every one of N simfiles must parse identically forever" (`AGENTS.md` §5) a first-class parametrised case, one reported entry per file — not a hand-rolled loop (doctest) or a fixture + `INSTANTIATE_TEST_SUITE_P` (gtest). |
| Float pinning | `WithinULP` / `WithinRel` matchers — `TimingData` characterisation wants ULP-level control, not a bare relative `Approx`. |
| Vendoring | Amalgamated = 2 files. gtest / full Catch2 = a source tree. |
| Lock-in | Macro vocabulary overlaps doctest ~90%; a later move either way is mechanical. |

Cost accepted: heavier per-TU include than doctest and a bigger vendored
blob. Mitigated by the amalgamated `.cpp` compiling once, not per test
TU, and by test TUs being few and small.

Not chosen: **doctest** (fastest build, but corpus ergonomics are a
hand-loop and float control is coarse); **GoogleTest** (best matchers +
mocking + death tests, but heaviest, most boilerplate for parametrised
tests, largest vendored footprint — revisit only if contributor
onboarding or seam-mocking becomes central).

## 2. Build: `src/` becomes an OBJECT library

`src/CMakeLists.txt` changes from

```cmake
add_executable(${SM_EXE_NAME} ${SMDATA_ALL_FILES_SRC} ...)
```

to

```cmake
add_library(sm_engine OBJECT ${engine_srcs} ...)      # all of src/ EXCEPT Main.cpp
add_executable(${SM_EXE_NAME} Main.cpp)
target_link_libraries(${SM_EXE_NAME} PRIVATE sm_engine)
```

and the new target:

```cmake
add_executable(sm_tests ${test_srcs} ${SM_EXTERN_DIR}/Catch2/catch_amalgamated.cpp)
target_link_libraries(sm_tests PRIVATE sm_engine)
```

Gated behind `option(WITH_TESTS "Build the unit-test target." OFF)` —
default OFF, ON in a dedicated CI job. Not built in a normal dev build.

### Split rules

- **Compilation settings** (`CXX_STANDARD`, `-Wall`/`/W4`, `WITH_WERROR`,
  SSE2, `/utf-8 /MP`, the MSVC `/Yc|/Yu global.h` PCH pairing, all
  `target_compile_definitions`) → **`sm_engine`**. Definitions that
  headers key on (`WINDOWS`, `DEBUG`, `UNIX`, …) are `PUBLIC` so both
  consumers inherit them; the warning/`-Werror` options stay `PRIVATE`
  (they must not be forced onto Catch2's TU or onto test sources).
- **Link inputs** (`SMDATA_LINK_LIB` — lua, pcre, zlib, ffmpeg import
  libs, `dbghelp`/`setupapi`/`hid`, …) → `target_link_libraries(sm_engine
  PUBLIC …)`. The engine objects reference these, so every consumer needs
  them; `PUBLIC` propagates them to the exe and to `sm_tests`.
- **Include dirs** → `target_include_directories(sm_engine PUBLIC …)`.
- **Executable-only** (output name/dir, `/SUBSYSTEM:WINDOWS`, `/MAP`,
  `/SAFESEH:NO`, `/NODEFAULTLIB:*`, the `mapconv` POST_BUILD, every
  `install()`) → stays on `${SM_EXE_NAME}`. `sm_tests` is a console
  binary and wants none of it.
- **The platform entry source** — `Main.cpp` everywhere, or
  `archutils/Darwin/SMMain.mm` on Apple — is **exe-only**, pulled out of
  `sm_engine` (`list(REMOVE_ITEM …)` for `SMMain.mm`, which
  `CMakeData-os.cmake` had put in the engine list) so its `main` never
  collides with Catch2's `main` in `sm_tests`. On Windows it also drops
  out of the PCH `foreach`, so it compiles without `/Yu` (one TU — no
  active PCH on macOS, so nothing to do there).

### Why an OBJECT library (not STATIC, not per-test source lists)

- **OBJECT vs STATIC:** avoids "symbol not referenced yet, drop it"
  archive semantics — every engine TU is always linked, matching today's
  `add_executable` behaviour exactly. No `--whole-archive` dance.
- **vs per-test source lists:** the engine is deeply tangled
  (`RageUtil.cpp` alone pulls Lua, pcre, json, `RageFile`,
  `RageSoundReader`). Hand-maintaining the source subset per test is a
  treadmill of link errors. Linking the whole engine once is simpler and
  honest about the current coupling.
- **Bonus, and on-mission:** the `sm_engine` boundary is the first real
  seam in a monolith. What a test can exercise without dragging in a
  subsystem becomes a *measurable* decoupling signal as modularisation
  proceeds.

# Consequences

- **Build structure change** — `src/CMakeLists.txt` gets ~60–80
  mechanical `${SM_EXE_NAME}` → `sm_engine` edits plus the split above.
  This is an `AGENTS.md` §4 large change: own branch, spelled-out
  spot-check, no merge until a green Windows Release build + `ctest`.
  Risk surface: MSVC PCH within `sm_engine`, `main` resolution,
  ffmpeg-w32 `LIBPATH` reaching `sm_tests` (handled via
  `target_link_directories(sm_engine PUBLIC …)`), `/SUBSYSTEM` staying
  off `sm_tests`.
- **CI** grows three jobs — `windows-tests` / `ubuntu-tests` /
  `macos-tests` (arm64): configure `-DWITH_TESTS=ON` (Windows also
  `-DWITH_WERROR=ON`), build `sm_tests`, `ctest --output-on-failure`.
  Separate from the ship builds so a red test never blocks the
  smoke/build signal while the suite is young. Windows is verified
  locally; the non-Windows `WITH_TESTS` paths are configure-checked and
  wait on a maintainer run (M1 / WSL) before the jobs are trusted.
- **`extern/Catch2/`** adds ~1.1 MB of vendored source. Bumping it =
  replace the two files + `LICENSE.txt`, note the version here and in
  `baseline.md`.
- **First coverage target:** the pure-ish cores, characterisation style
  (pin what the code does *now*, bug-for-bug) — `RageUtil` string/path
  helpers first, then `RageMath`, then `TimingData` beat/second
  conversions, then a `NotesLoader` smoke over a tiny committed simfile
  corpus. Salvage the *intent* of the old `test_timing_data` /
  `test_file_readers` where still meaningful.
- **`RString`:** tests include engine headers, so `RString` is in the
  signatures under test. Fine — tests pin behaviour through the current
  API and move with it as the migration proceeds.
- Reversible: delete `tests/`, revert the `src/CMakeLists.txt` split,
  drop the CI job and `extern/Catch2/`. The OBJECT-library shape is
  worth keeping on its own merits (incremental-build wins) even if the
  framework choice were revisited.

# Phases

1. **Scaffold** (this ADR, branch `feature/test-harness`): vendor Catch2;
   OBJECT-library split; `WITH_TESTS` option; `tests/CMakeLists.txt`;
   `tests/test_RageUtil.cpp` as the first real coverage; CI job.
   Configure-validated locally; maintainer build-verifies + merges.
2. `RageMath` + `RageUtil` numeric/path helpers; a `tests/README` and a
   `playbooks/add-characterization-test.md`.
3. **DONE** (2026-09-05) — `TimingData` conversions,
   `NoteData`/`NoteDataUtil` transforms; see `baseline.md`.
4. **`.sm`/`.ssc` DONE** (2026-09-06) — `GENERATE(from_range(...))`
   parse-regression in `tests/test_NotesLoaderCorpus.cpp` over the
   **real committed SM5 sample songs** (`Songs/StepMania 5/…`), via the
   `EngineTestEnv` fixture below. Toy simfiles are not used — a toy only
   proves the loader survives input its author understood, not the
   §5 invariant (`tests/data/README.md`). Feeds `AGENTS.md` §5 for the
   canonical read + write formats, including a cross-format equivalence
   check (Goin' Under `.sm` vs `.ssc`). **Still open:** `.sma` /
   `.dwi`/`.ksf`/`.bms` / `.crs` — `EngineTestEnv` now has `PREFSMAN`
   (2026-09-06), so `LoadFromDir` is reachable; each format just needs
   a committed sample song under `Songs/` (none exists yet). `.crs`
   likely also needs `SONGMAN`. Backlog item 17.

## Phase 3-4 enabler: `tests/EngineTestEnv` (2026-09-06)

Phases 3-4 and the salvage of `src/tests/test_file_readers.cpp` /
`test_audio_readers.cpp` were all blocked on the same thing: a shared
way to bring up the engine singletons a test needs to *log*, *read a
file*, or *touch Lua*, without booting the whole engine.

`tests/EngineTestEnv.{h,cpp}` is that fixture. `EngineTestEnv::Require()`
idempotently constructs, once per `sm_tests` process:

| Global | Why | Order constraint |
|---|---|---|
| `LUA` (`LuaManager`) | `RageFileManager`'s ctor calls `LUA->Get()` | first |
| `FILEMAN` (`RageFileManager`) | `RageFile` I/O; mounts `tests/data/` at `/testdata` and the repo `Songs/` at `/Songs` | after `LUA` |
| `LOG` (`RageLog`) | error branches call `LOG->UserLog`/`LOG->Warn` | after `FILEMAN` (its ctor opens a `RageFile`, which `ASSERT`s `FILEMAN`) |
| `PREFSMAN` (`PrefsManager`) | dir-only loaders read it (`DWILoader` → `m_bQuirksMode`, courses → `m_bFastLoad`); Song/Steps paths too | after `LUA` + `FILEMAN` (ctor registers with `LUA`, reads `Data/*.ini` via `FILEMAN` — none mounted, so compiled defaults stand). Dtor calls `LUA->UnsetGlobal` → torn down before `LUA` |
| `GAMEMAN` (`GameManager`) | `#STEPSTYPE` → `StepsType` resolution in every real load | after `LUA`; ctor is trivial (Lua registration only — the game/style/`StepsType` tables are file-scope static data) |

A `CATCH_REGISTER_LISTENER` tears them down at `testRunEnded`, in reverse
construction order. Tests that never call `Require()` are unaffected.
Deliberately **not** provided: `GAMESTATE`, `THEME`, `SONGMAN`, renderer,
audio. `Song::LoadFromSongDir` (the full song-directory load, with the
cache) still needs more than this; `LoadFromSimfile` for `.sm`/`.ssc`/
`.sma` and `LoadFromDir` for `.dwi`/`.ksf`/`.bms` are now reachable —
the only thing missing for the dir formats is a committed sample song
(backlog item 17).
Paths reach the fixture through a `file(GENERATE)`d `EngineTestEnvPaths.h`
(raw string literals, so Windows backslashes need no escaping).

Consumers:
- `tests/test_NotesLoaderFull.cpp` — the `SMLoader::ParseBPMs`/
  `ParseStops` log-and-skip branches + `LoadFromSimfile` on a missing
  path (error branches that need a live `LOG`, no file).
- `tests/test_NotesLoaderCorpus.cpp` — **phase 4 proper**: a
  `GENERATE(from_range(...))` parse-regression over the real committed
  SM5 sample songs (`Songs/StepMania 5/{Goin' Under, MechaTribe
  Assault, Springtime}`), pinning per-song metadata + song BPM and
  per-chart (file order) `StepsType`/difficulty/meter/track-count/
  tap-count — 39 charts across the 4 files — plus a Goin' Under
  `.sm`-vs-`.ssc` equivalence case. Characterization values captured
  from a hidden `[.dump]` case in the same file (`sm_tests "[dump]"`
  to re-baseline). This is the `AGENTS.md` §5 invariant in test form.
