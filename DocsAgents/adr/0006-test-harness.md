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
   check (Goin' Under `.sm` vs `.ssc`).
   **`.pms` / BMS family DONE** (2026-09-06, `test_NotesLoaderBMS.cpp`)
   via `BMSLoader::LoadFromDir` — the fixture (`tests/data/pms-fixture/`)
   is **derived, not redistributed**: real 3-chart `.pms` structure
   (note data, timing, `#BPM`/`#WAVxx` channels) byte-for-byte, with
   title/artist/genre + `#WAV` filenames scrubbed and keysound audio
   swapped for 44-byte silent stubs (`tests/data/README.md`). Covers
   `pnm-five` / `pnm-nine` / `bm-double7`.
   **`.dwi` DONE** (2026-09-06, `test_NotesLoaderDWI.cpp` +
   `tests/data/dwi-fixture/`) — `DWILoader::LoadFromDir` over a derived
   fixture (only `#FILE`/`#TITLE`/`#ARTIST` changed from source; DWI
   has no keysounds). Pins the `" ("` main/sub split, `#GAP`→offset,
   `#BPM` + `#CHANGEBPM`, `#SAMPLESTART`, 3 `dance-single` charts.
   **`.ksf` DONE** (2026-09-08, `test_NotesLoaderKSF.cpp` +
   `tests/data/Fixture Artist - KSF Fixture/`) — `KSFLoader::LoadFromDir`
   over a derived 4-chart Pump It Up fixture (only `#TITLE`/`#ARTIST`/
   `#STEPMAKER`/`#SONGFILE` scrubbed). Needs `song.SetSongDir()` (no
   `Dirname` fallback); the artist comes from the dir name (KSFLoader
   ignores `#ARTIST`); the filename drives type+difficulty. Covers
   `pump-single` + `pump-double`.
   **`.sma` DONE** (2026-09-10, `test_NotesLoaderSMA.cpp` +
   `tests/data/sma-fixture/`) — `SMALoader::LoadFromDir` over a
   **synthetic** fixture. No real `.sma` file exists anywhere (extinct
   2009-2011 SMA-editor format); after an extended search the maintainer
   found none, and decided the current read behavior is the reference.
   The synthetic fixture pins the SMA-only tags: `#ROWSPERBEAT` row⇔beat
   translation (`8r` → row/4, bare `24` → beat 24), `#BEATSPERMEASURE`
   (with the 4/4 back-fill), `#SPEED` `s`-suffix → `UNIT_SECONDS`,
   `#MULTIPLIER` 2-vs-3-field combo/miss. Also pins the quirk that
   `#BPMS`/`#STOPS` land on the *Steps* timing (empty at song level)
   while pre-`#NOTES` tags land on song timing.
   **`.crs` DONE** (2026-09-10, `test_NotesLoaderCRS.cpp`) —
   `CourseLoaderCRS::LoadFromBuffer` (→ `LoadFromMsd`, `bFromCache=true`)
   over inline course text, no fixture files. Pins metadata (`#COURSE`,
   `#SCRIPTER`, `#REPEAT`, `#LIVES`, `#BANNER`, `#METER` 2- and 3-param),
   `#SONG` entry resolution against the empty `SONGMAN` (`BEST1`
   accepted / `BEST2` rejected — off-by-one; `GRADEBEST`/`*` accepted;
   1-part title miss rejected), old-style difficulty aliases + `lo..hi`
   meter ranges + the `3..6` fallback, and the `#SONG` modifier-column
   keywords. Writing it surfaced a real bug, **now fixed**: the
   recognised-tag guard was `else if( !eq(A) || !eq(B) || !eq(C) )`
   (always true), so `#STYLE`, the RADAR-cache branch and the
   "unexpected value" log were all dead — `#STYLE` on a course did
   nothing. Fixed to `eq(A) || eq(B) || eq(C)` and moved the `#STYLE`
   handler above the `bFromCache` catch-all; the test now checks
   `#STYLE` populates `m_setStyles`. Still not covered: 2-part
   `#SONG:Group/Song` refs (resolving them needs `PROFILEMAN`, null in
   the fixture — not a real-engine bug).
   **ADR 0006 phase 4 is now complete for every simfile/course format
   the engine loads.**

## Phase 3-4 enabler: `tests/EngineTestEnv` (2026-09-06)

Phases 3-4 and the salvage of `src/tests/test_file_readers.cpp` /
`test_audio_readers.cpp` were all blocked on the same thing: a shared
way to bring up the engine singletons a test needs to *log*, *read a
file*, or *touch Lua*, without booting the whole engine.

`tests/EngineTestEnv.{h,cpp}` is that fixture. `EngineTestEnv::Require()`
idempotently constructs, once per `sm_tests` process:

| Global / step | Why | Order constraint |
|---|---|---|
| `LUA` (`LuaManager`) | `RageFileManager`'s ctor calls `LUA->Get()` | first |
| `ActorUtil::InitFileTypeLists()` | populates the static extension↔filetype maps; `RageSoundReader_FileReader::OpenFile` (and image/movie readers) consult them | after `LUA`; manager-free; not idempotent, so relies on `BringUp()` running once |
| `FILEMAN` (`RageFileManager`) | `RageFile` I/O; mounts `tests/data/` at `/testdata` and the repo `Songs/` at `/Songs` | after `LUA` |
| `LOG` (`RageLog`) | error branches call `LOG->UserLog`/`LOG->Warn` | after `FILEMAN` (its ctor opens a `RageFile`, which `ASSERT`s `FILEMAN`) |
| `PREFSMAN` (`PrefsManager`) | dir-only loaders read it (`DWILoader` → `m_bQuirksMode`, courses → `m_bFastLoad`); Song/Steps paths too | after `LUA` + `FILEMAN` (ctor registers with `LUA`, reads `Data/*.ini` via `FILEMAN` — none mounted, so compiled defaults stand). Dtor calls `LUA->UnsetGlobal` → torn down before `LUA` |
| `PREFSMAN` also mounts the repo `Themes/` at `/Themes` and `NoteSkins/` at `/NoteSkins` | so `ThemeManager`/`NoteSkinManager` can list them (relative `"Themes/*"` resolves against the VFS root) | after `FILEMAN` |
| `MESSAGEMAN` (`MessageManager`) | trivial ctor; also stops `LuaHelpers`' script-error path (`ScriptErrorMessage` → `MESSAGEMAN->Broadcast`) from dereferencing null | after `LUA`; before `GAMESTATE` (matches `sm_main`) |
| `GAMESTATE` (`GameState`) | many `GAMESTATE->` paths; **ctor only, not `Reset()`** (its ctor skips that deliberately) so `GetCurrentGame()` etc. are still unsafe | after `MESSAGEMAN` |
| `GAMEMAN` (`GameManager`) | `#STEPSTYPE` → `StepsType` resolution in every real load | after `LUA`; ctor is trivial (Lua registration only — the game/style/`StepsType` tables are file-scope static data) |
| `THEME` (`ThemeManager`) | **constructed only — no `SwitchThemeAndLanguage()`** | after `GAMEMAN` |
| `NOTESKIN` (`NoteSkinManager`) | trivial ctor; non-null unblocks the `PlayerOptions` mod-processing `ASSERT` | after `THEME` |
| `SONGMAN` (`SongManager`) | ctor registers + `Load()`s a few `ThemeMetric`s (no-op while no theme is loaded); **`InitAll()` NOT called** — holds no songs/courses | after `THEME` |

A `CATCH_REGISTER_LISTENER` tears them down at `testRunEnded`, in reverse
construction order (every manager whose dtor calls `LUA->UnsetGlobal` is
destroyed before `LUA`). Tests that never call `Require()` are unaffected.

**`THEME` is constructed but no theme is switched in.**
`SwitchThemeAndLanguage()` runs the whole theme's Lua
(`Themes/{_fallback,default}/Scripts/*.lua`) and refreshes the
screen-dimension metric cache, and that path **SIGSEGVs in the headless
harness** — theme code assumes a live engine (renderer, `SCREENMAN`,
sound). So `THEME != nullptr` (unblocks anything that only needs the
pointer), but `THEME->IsThemeLoaded()` is false and every
`ThemeMetric<T>` / `CommonMetrics::*` stays unset — a test that reads an
actual metric *value* still hits the `m_Value.IsSet()` assert. A
headless way to load metric *values* (a scripts-free minimal test theme,
or a stub metric provider) is tracked under backlog item 17.

Still not provided at all: the renderer and the audio device.
`Song::LoadFromSongDir` (the full song-directory load, with the cache)
still needs more than this; `LoadFromSimfile` for `.sm`/`.ssc`/`.sma`
and `LoadFromDir` for `.dwi`/`.ksf`/`.bms` are reachable.
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
- `tests/test_NotesLoaderBMS.cpp` — `BMSLoader::LoadFromDir` over the
  derived `.pms` fixture (`tests/data/pms-fixture/`): 3 charts
  (`pnm-five` / `bm-double7` / `pnm-nine`), keysound-file count,
  metadata, BPM. Hidden `[bmsdump]` case to re-baseline.
- `tests/test_NotesLoaderDWI.cpp` — `DWILoader::LoadFromDir` over the
  derived `.dwi` fixture (`tests/data/dwi-fixture/`): 3 `dance-single`
  charts, `#GAP`/`#BPM`/`#CHANGEBPM`/`#SAMPLESTART`, the `" ("`
  main/sub title split. Hidden `[dwidump]` case.
- `tests/test_NotesLoaderKSF.cpp` — `KSFLoader::LoadFromDir` over the
  derived `.ksf` fixture (`tests/data/Fixture Artist - KSF Fixture/`):
  4 `pump-single`/`pump-double` charts, `#BPM`, `#STARTTIME`→offset;
  pins that artist comes from the dir name. Hidden `[ksfdump]` case.
- `tests/test_NotesLoaderSMA.cpp` — `SMALoader::LoadFromDir` over a
  **synthetic** `.sma` fixture (`tests/data/sma-fixture/`, no real file
  exists — maintainer decision 2026-09-10). Pins the SMA-only tags
  (`#ROWSPERBEAT` `r`-suffix row translation, `#BEATSPERMEASURE` 4/4
  back-fill, `#SPEED` `s`→seconds unit, `#MULTIPLIER` combo/miss) and
  the song-vs-Steps timing split. Hidden `[smadump]` case.
- `tests/test_NotesLoaderCRS.cpp` — `CourseLoaderCRS::LoadFromBuffer`
  over inline course text (no fixture files; `bFromCache=true` path, so
  no SONGINDEX/cache probe). Pins course metadata, `#SONG` entry
  resolution vs an empty `SONGMAN` (the `BEST<n>` off-by-one), the
  difficulty/meter-range parsing, and the `#SONG` modifier keywords.
  Its `#STYLE` case pins the fix for the dead recognised-tag guard
  (`!eq||!eq||!eq`, always true — `#STYLE` used to do nothing). 2-part
  `Group/Song` refs stay uncovered (need `PROFILEMAN`). Hidden
  `[crsdump]` case.
- `tests/test_RageFile.cpp` — `RageFile` read/write/seek/tell/`GetLine`/
  `AtEOF` through `FILEMAN`'s writable `/@mem` mount (no committed
  fixtures). Salvages `src/tests/test_file_readers.cpp`; pins the
  stdio-like EOF semantics.
- `tests/test_RageFileDeflate.cpp` — `RageFileObjDeflate` /
  `RageFileObjInflate` raw-deflate round-trip over `RageFileObjMem`:
  byte-exact + CRC32(input)==CRC32(output) at several block sizes,
  `RageFileObjInflate::Seek`, "deflate shrinks". Salvages
  `TestDeflate()` from `src/tests/test_file_readers.cpp`.
- `tests/test_RageFileErrors.cpp` — a self-registering in-test VFS
  driver ("ERRTEST") that fails the read/write/flush crossing a byte
  threshold. Pins that mid-stream driver errors propagate as
  `RageFile::Read`/`Write` → -1 with `GetError()` set, that `Flush()`
  surfaces `FlushInternal`'s error, and that `IniFile::ReadFile`/
  `WriteFile` carry it up. Salvages `src/tests/test_file_errors.cpp`.
- `tests/test_Zip.cpp` — `RageFileDriverZip` (read-only ZIP VFS). The
  test archive is assembled by a ~90-line in-file minimal ZIP writer
  (one STORED entry + one DEFLATED entry compressed with
  `RageFileObjDeflate`), written to `/@mem`, then read back: entries
  byte-exact, CRC32 + method + size fields parsed correctly, missing
  entry → nullptr, `Open(WRITE)` → `ERROR_WRITING_NOT_SUPPORTED`,
  non-zip → `Load` false. (The engine has no ZIP *writer* — `CreateZip`
  was dead STORED-only code, deleted; see backlog item 19.)
- `tests/test_RageSurface.cpp` — `RageSurface` + `RageSurfaceUtils` pure
  helpers (built in memory, no image files / GL):
  `decode/encodepixel`, `Set/GetRawRGBAV`, `GetBitsPerChannel`,
  `RageSurfaceFormat::operator==`/`Equivalent` (regression pin for the
  `f5005b8754` palette-`memcmp` fix), `Blit`, `ConvertSurface`.
- `tests/test_RageSoundReader.cpp` — the WAV decoder from a synthetic
  PCM WAV written to `/@mem` (no fixture). Both
  `RageSoundReader_WAV::Open` and the `OpenFile` autodetect factory.
  Salvages `src/tests/test_audio_readers.cpp`.
- `tests/test_IniFile.cpp` — the `.ini` reader/writer (`Preferences.ini`
  et al.), parsed from strings via `/@mem`. Pins the parse quirks
  (trimmed key / untrimmed value, comment prefixes, pre-section drop,
  `\`-continuation) and `RenameKey`/`DeleteKey`/round-trip.
- `tests/test_XmlFile.cpp` — the hand-rolled XML parser
  (`XmlFileUtil::Load`/`GetXML` over `XNode`), from strings (no
  fixture, no `FILEMAN`). Pins text-only-before-first-child, the
  five-named-entities-only decode, unquoted/name-only attrs, prolog +
  comment skipping, `GetXML` round-trip, error strings.
- `tests/test_DateTime.cpp` — the y/m/d/h/m/s value type. `GetString`
  omits an all-zero time; `FromString` round-trips it and does **not**
  validate the fields; comparison ordering.
- `tests/test_GameManager.cpp` — `StringToStepsType` / `StringToGame` /
  `GameAndStringToStyle` table lookups + `GetStylesForGame` /
  `GetStepsTypesForGame` (GAMEMAN from the fixture).
- `tests/test_RageColor.cpp` — the `"1,0,0.5"` / `"#FF8000"` colour
  codec; parse failure resets to opaque white; `ToString` /
  `NormalizeColorString`.
- `tests/test_SongOptions.cpp` — the per-song mod set
  (`xMusic` / clap / autosync* / haste), unknown-mod ignore,
  `GetString`/`FromString` round-trip. (`PlayerOptions` needs
  `NOTESKIN` — not yet in `EngineTestEnv`.)
- `RageMath` bezier: `test_RageMath.cpp` also covers `RageQuadratic`
  shortcuts + `RageBezier2D::EvaluateYFromX`.
- `RageUtil` conv: `test_RageUtil.cpp` also covers
  `StringConversion::FromString`/`ToString<T>` (the `Preference<T>`
  codec).
