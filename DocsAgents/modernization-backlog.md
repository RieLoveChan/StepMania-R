---
type: Reference
title: Modernization backlog
description: Ranked list of code needing attention, from a 2026-09-02 source-tree sweep. The live to-do for the fork.
tags: [modernization, backlog, triage]
---

# How to use

Ranked by "does this block safe continuous work" first, then rot, then
risk, then hotspots. Tiers 1–2 are step 2 (make change safe + cheap
cleanup). Tier 3 needs ADR decisions. Tier 4 is where ongoing passes go.

Update this file as items are closed (strike through + link the PR) and
as new sweeps find things. Numbers/anchors confirmed 2026-09-02.

---

## Tier 1 — Blocks safe continuous work

### 1. Safety net — smoke DONE, harness scaffold DONE, coverage OPEN
- **Headless smoke: DONE** (`f7249f3a95`) — `--SelfTest` flag runs full
  engine init and exits 0; wired into Windows CI (`continue-on-error`
  until it's green a few times, then make fatal).
- **Harness scaffold: DONE** (merged `5_1-new` 2026-09-04). ADR
  [0006](./adr/0006-test-harness.md) (Catch2 v3 + `sm_engine` OBJECT
  library); `sm_tests` + first `RageUtil` coverage; CI green on
  Windows/macOS/Linux (see item 17).
- **Unit coverage: OPEN.** Characterization targets, in order:
  `RageUtil` (started) → `RageMath` → `TimingData` →
  `NoteData`/`NoteDataUtil` → `NotesLoader*` (tiny committed corpus).

### 2. Warnings on but unmeasured / unenforced
`-Wall -Wextra` (GCC/Clang) + `/W4` (MSVC) with blanket suppressions
(`src/CMakeLists.txt:126-132`); no `-Werror` except `type-limits`.
**Action:** capture counts into [`baseline.md`](./baseline.md); add a
`WITH_WERROR` option (default OFF, ON in CI) and promote each warning
category to `-Werror=<cat>` once it hits zero across `src/` (ADR 0001 §7).

### 3. Stale cppcheck leak list, never cleared
`Docs/Devdocs/possible memory leaks.txt` — from 2009. Likely still live:
`ActorFrameTexture::m_pRenderTarget`, `AdjustSync::s_pTimingDataOriginal`,
`AutoKeysounds::m_pSharedSound`, `LifeMeterTime::m_pStream`,
`MusicWheelItem::m_pTextSectionCount`, `OptionRow::m_textTitle`,
`Font.cpp` `pPage`, `RageFileDriverDeflate.cpp`.
**Action:** re-run cppcheck/ASan, confirm which survive, fix or dismiss
with a note. Small, independent PRs.

---

## Tier 2 — Rot / dead weight (cheap, low-risk)

### 4. Dead CI committed — DONE 2026-09-03 (`e065f69c8b`)
~~`.travis.yml` (travis-ci.org shut 2021) + `.appveyor.yml` (VS2015,
v140_xp, IRC reporter, stale token). Removed. Live CI is
`.github/workflows/ci.yml`.~~

### 5. Dead IRC notifier — DONE 2026-09-03 (`718d3b3ec1`)
~~`src/CMakeProject-irc.cmake` + `src/irc/appveyor.cpp` — orphaned
(not `include()`d anywhere), targeted defunct `irc.freenode.net`.
Removed.~~

### 6. Stale build docs — DONE 2026-09-03
~~`Build/README.md` said "CMake min 2.8.12", "latest 3.3.0-rc3"; actual is
CMake 3.20 + C++17. Rewrote `Build/README.md` and `Build/INSTALL.md` to
current reality (`cmake -B build`, VS 2022, Windows 11 floor, no dev
install step, `StepMania-R` exe names). Fixed the matching 2.8.12 line and
`Build/StepMania.sln` path in `DocsAgents/build.md`. Root `README.md`
Travis badges were already removed.~~

---

## Tier 3 — Risk; deliberate decisions (see ADR 0001, ADR 0003)

### 8. Unsafe C string ops in crash/URL/zip paths
~40 `strcpy`/`strcat`/`strncpy` into fixed buffers, concentrated in
`archutils/Win32/Crash*.cpp`, `archutils/Unix/CrashHandler*`. Genuine
overflow candidates: `Crash.cpp:30` `strcpy(pszFile, fn)`, `:170`;
`archutils/Win32/GotoURL.cpp:22,59-60` (builds a shell command with
`strcat` + a URL); `CreateZip.cpp` `strcpy`/`strcat` into `zfi.name`
etc. with user paths.
**Action:** Windows crash + `GotoURL` + `CreateZip` are in scope
(P1 platform). Signal-handler context partly justifies no-alloc, but
bounded copies with explicit length checks are still required. Treat as
`AGENTS.md` §4 large changes (touch the crash path → verify carefully).

---

## Tier 4 — Hotspots (where ongoing passes concentrate; not bugs)

### 9. God objects / oversized TUs
`ScreenEdit.cpp` 6596 · `GameManager.cpp` 3614 · `Player.cpp` 3567
(30 TODO/HACK) · `GameState.cpp` 3523 (~2,100 `GAMESTATE->` call sites) ·
`ScreenGameplay.cpp` 3381 · `NoteDataUtil.cpp` 3379 · `Profile.cpp` 2897.
**Action:** [`playbooks/split-god-object.md`](./playbooks/split-god-object.md),
one cluster per PR, always §4.

### 10. RString everywhere
`typedef StdString::CStdString RString` (`global.h:107`), 723 files /
~8,429 uses. Declared retirement goal (ADR 0001 Settled #5).
**Action:** [`playbooks/migrate-rstring.md`](./playbooks/migrate-rstring.md),
per subsystem, opportunistic.

### 11. Pre-C++11 threading / smart pointers
`RageThreads` predates `std::thread`/`std::mutex`;
`RageUtil_AutoPtr.h` ("TODO: replace with c++11 smart pointers");
`RageUtil_WorkerThread`, `BackgroundLoader`.
**Action:** after the safety net exists; `RageThreads` is load-bearing
and cross-platform — a dedicated ADR-scoped effort, not a casual pass.

### 12. Mechanical modernize-* debt
`modernize-use-nullptr`, `-use-override`, `-use-equals-default`,
`-use-bool-literals`, redundant void args, etc. across the tree.
**Action:** [`playbooks/clang-tidy-subsystem-pass.md`](./playbooks/clang-tidy-subsystem-pass.md),
one subsystem + one check family per PR; record in
[`baseline.md`](./baseline.md).

### 13. `src/archutils/Win32/arch_setup.h` legacy — mostly DONE
- `isnan`/`isfinite` macros removed 2026-09-03 (`37e6766d5e`).
- `_WIN32_WINNT 0x0601` → `0x0A00`, `_WIN32_IE 0x0400` → `0x0A00`,
  `#define __STDC__ 0` removed, Win98/ME comment dropped — 2026-09-03
  (`5565039bf7`). Clean rebuild; runtime not yet smoke-tested.
**Remaining (low priority, warning-suppression territory — bundle with a
warnings pass):** `_CRT_SECURE_NO_DEPRECATE` / `_SCL_SECURE_NO_DEPRECATE`
(the latter is a no-op on VS2017+; the former is redundant with the
CMake-level `_CRT_SECURE_NO_WARNINGS`), the stale VC6/VC2005 comment
block (lines 9-36).
Related: `src/archutils/Win32/DirectXErrorList.h` — 12 `case` labels
(`0x8007xxxx`) that don't fit signed `HRESULT`; MSVC compiles it, clang
rejects (C++11 narrowing). Rewrite the cases as hex literals / `HRESULT(...)`.

### 17. Pick a unit-test framework + write core characterization tests — scaffold DONE (ADR 0006 phase 1), phases 2-4 open
Framework decided: **Catch2 v3** (amalgamated, vendored `extern/Catch2/`
@ v3.16.0) — ADR [0006](./adr/0006-test-harness.md). Build approach:
`src/` → OBJECT library `sm_engine`, shared by the exe and a new
`sm_tests` target, behind `WITH_TESTS` (default OFF, CI-on).
**Scaffold merged to `5_1-new` (2026-09-04):** `src/CMakeLists.txt`
OBJECT-library split, `tests/CMakeLists.txt`, `tests/test_RageUtil.cpp`
(first characterization coverage), Apple `SMMain.mm` entry-point split,
CI jobs for Windows/macOS/Linux × `sm_tests`. All 8 CI jobs green; local
Windows Release build + `--SelfTest` also verified (§4 gate). Fixed en
route: `LoadingWindowGtk` OBJECT → STATIC (Linux-only link fix, see
`log.md` 2026-09-04).
**Remaining (ADR 0006 phases 2-4):** `RageMath` + `RageUtil`
numeric/path helpers → `TimingData` (`WithinULP`) → `NoteData`/
`NoteDataUtil` → a tiny committed simfile corpus for `NotesLoader*` via
`GENERATE(from_range(...))`. Salvage the intent of the old
`test_timing_data` / `test_file_readers` / `test_misc` where still
meaningful.

### 16. Pre-floor `#if` guards across `src/arch/` and `src/archutils/`
Now that ADR 0003 sets Windows 11 / current-macOS / current-Linux floors,
sweep for `#if`/`#ifdef` guards handling below-floor OSes: `_WIN32_WINNT`
comparisons, `WINVER` checks, `MAC_OS_X_VERSION_MIN_REQUIRED` for old
10.x, XP/9x fallback branches, 32-bit paths, EOL-distro `#ifdef`s.
**Action:** batched sweep, one `src/arch/<area>` per commit, Windows
build verified. Coordinate with items 13 (`arch_setup.h`) and the
D3D9/GLES2 question (ADR 0004).

### 15. `#if 0` dead blocks
~37 `#if 0` markers across `src/` (`Player.cpp`, `NoteDataUtil.cpp`,
`ScreenEdit.cpp`, `RageDisplay_GLES2.cpp`, `ScoreKeeperNormal.cpp`, …).
**Action:** a dedicated sweep — glance at each (some guard
kept-for-reference code), remove the truly dead ones. One commit per
handful of files.

### 14. `vcvars64.bat` does not wire the Windows SDK on the maintainer box
`vcvars64.bat` sets only the MSVC toolchain INCLUDE/LIB, not the Windows
SDK (`WindowsSdkDir` empty, `WindowsSDKLibVersion=winv6.3\`). The VS
generator build works anyway (MSBuild finds the SDK via VS props); Ninja
/ command-line builds need INCLUDE/LIB reconstructed by hand (SDK
`10.0.26100.0` at `C:\Program Files (x86)\Windows Kits\10`).
**Action:** low priority — repair the VS install / SDK registration, or
ship a project `env` helper script. Not blocking (workaround documented
in `baseline.md` → "How to (re)generate").

### 18. Logging overhaul — phase 1 DONE, phases 2-4 open
Phase 1 (`c82d0e9058`): bracketed level tags, `Error()` level, no
`/////`, `Char Widths` fixed. `--SelfTest` log 695→467 lines, clean.
**Remaining (ADR [0005](./adr/0005-logging-overhaul.md)):**
- Ph2: `Debug` level, per-category thresholds + `--LogLevel`,
  `Log::Category` enum, `LOG_*` macros capturing `file:line`.
- Ph3: repeat-collapsing (`… (repeated N×)`).
- Ph4: call-site audit per subsystem — `Warn`→`Trace` (expected
  fallback) / `Warn`→`Error` (real failure), add category tags.

### 19. OS version detection reports "Windows 8"
`info.txt`: `Windows 6.2 (Win8) build 9200` on a Windows 11 box.
`GetVersionEx`-style detection without a `supportedOS` entry in the app
manifest caps at 6.2. `src/archutils/Win32/` OS detection + the exe
manifest. Cosmetic but misleading in crash reports; fits the ADR-0003
Win11-floor work (item 16).

### 20. Replace the archaic hard-coded game-type system
Game types are defined by hand-written `static const Game g_Game_X = {…}`
struct literals in `src/GameManager.cpp` (~150 lines each: controllers,
button maps, per-style mappings, menu buttons), registered in a
hand-maintained `g_Games[]` array, with a **compile-time `StepsType`
enum** and a parallel `g_StepsTypeInfos[]` array. Adding or editing a game
means editing a 3600-line `.cpp` and rebuilding. All game types are now
enabled (`229d0769d5`); the *mechanism* is what needs replacing.
**Goal:** a **data-driven game/style/stepstype registry** — defined in
data (Lua or a `Games/` tree, like NoteSkins/Themes are), loaded at
startup, with the C++ side working off a runtime id instead of the
`StepsType` enum.

**End-state — `NoteSkins/` is the switch for supported games.** A game is
offered iff `NoteSkins/<gamename>/` holds ≥1 valid skin. This is *already*
how enablement works (`GameManager::GetEnabledGames` →
`IsGameEnabled` → `NoteSkinManager::DoNoteSkinsExistForGame`); after
`229d0769d5` `g_Games[]` lists all defined games, so the NoteSkins check
is the only gate that reaches the UI. What remains: the `g_Games[]` array
+ `g_Game_*` structs are still hand-maintained C++. Target: adding a game
= drop in a definition file + a `NoteSkins/<name>/` folder — no `.cpp`
edit, no rebuild. `g_Games[]` becomes "every definition found", the
NoteSkin dir stays the on/off.
**Hard constraint:** the on-disk `#STEPSTYPE` strings (`dance-single`,
`pump-double`, `bm-single7`, `pnm-nine`, …) are a stable contract with the
~20-year simfile library (`AGENTS.md` §5) — every existing value must
resolve identically, no mass cache invalidation.
**Scope:** big. `StepsType` (enum → runtime id) ripples through
`NoteData`, `Style`, `Steps`, `RadarValues`, score keepers, the editor,
Lua bindings, `Profile` serialization. **Deserves its own ADR** when
picked up. Also: `NoteSkins/Para/` is capitalised but the game name is
`para` — rename for Linux (case-sensitive FS).

---

## Closed

- **Item 7** — `extern/ffmpeg-w32/` (36 MB committed blob, unknown
  provenance) replaced with a CI-built artifact from the pinned
  `extern/ffmpeg` submodule (2026-09-04). `.github/workflows/
  build-ffmpeg-win32.yml` cross-compiles with mingw-w64 on
  `ubuntu-latest` (no Windows runner needed) and generates MSVC `.lib`
  import libs via `gendef` + `llvm-lib`; published as GitHub Release
  `ffmpeg-w32-19feb712f5`. `CMake/SetupFfmpegWin32.cmake` downloads +
  SHA256-verifies it into `extern/ffmpeg-w32-prebuilt/` at configure
  time (offline on repeat configures). `StepmaniaCore.cmake`,
  `src/CMakeLists.txt`, `tests/CMakeLists.txt` point at that dir now.
  Verified locally (fresh network download, Release build + `--SelfTest`,
  `WITH_TESTS` Debug build) and in CI (all 8 jobs green, including the
  Windows runner downloading the same Release asset). Deviates from the
  original recipe by dropping `--enable-bzlib`/`-zlib` (no Ubuntu
  mingw-w64 package for either; unused by StepMania's codec paths).
- **Item 4** — dead Travis + AppVeyor CI configs removed (`e065f69c8b`, 2026-09-03).
- **Item 5** — orphaned `src/irc/` IRC-reporter subproject removed (`718d3b3ec1`, 2026-09-03).
- **Item 6** — `Build/README.md` + `Build/INSTALL.md` rewritten to current
  reality (CMake 3.20, `cmake -B build`, Windows 11 floor); `build.md`
  cross-checked (2026-09-03).
- **Item 13 (mostly)** — `arch_setup.h`: dead `isnan`/`isfinite` macros
  (`37e6766d5e`); `_WIN32_WINNT`/`_WIN32_IE` → `0x0A00`, `__STDC__ 0`
  removed (`5565039bf7`). Only warning-suppression cruft left.
