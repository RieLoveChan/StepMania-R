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

### 2. Warnings on but unmeasured / unenforced — ratchet turning, 2 of 5 MSVC categories promoted
`WITH_WERROR` exists (default OFF, ON in Windows CI) and counts are
captured in [`baseline.md`](./baseline.md) (ADR 0001 §7 mechanism).
**Done (2026-09-04):** `C4189` (unused local, was 26) and `C4702`
(unreachable code, was 10) promoted to `-Werror` — all 15 unique hit
sites fixed by hand first (dead locals removed; two fully-dead
computation blocks deleted; degenerate `FOREACH_X(v) return ...;`
loops — which MSVC flags because the body always returns on the first
iteration, making the loop's back-edge provably unreachable — rewritten
as a direct `GetNextX()` + `if`; one `[[noreturn]]`-followed-by-dead-code
cleanup). Verified clean under `/WX` in both Debug and Release (Debug
surfaced 3 more sites Release's optimizer had folded away — always
check both configs, not just Release).
**In progress (2026-09-05):** `C4100` (unreferenced parameter). Real
measured count is **314 unique sites** (not the stale ~1362 raw-line
figure, which double-counts headers across every including TU) spread
across ~150 files with no concentration (max 11 in one file) — a wide
mechanical sweep, no shortcut. **198 of 314 fixed** (`713e58a0f6` +
`9844ab3c4b` + `26b7de158b`), the 12 highest-concentration files, then
the 19 files at exactly 4 sites each, then 13 files at exactly 3 sites
each: comment out the unused parameter name (`Type /* name */`, this
codebase's existing convention), except one site genuinely used only
under `#if defined(HAVE_POSIX_FADVISE)` (unset on Windows) —
`[[maybe_unused]]` there instead, since commenting the name would
break the other platform's compile. Several sites looked like false
positives but weren't: verify each site's actual body rather than
assume the pattern — one function's "unused" param was used by a
*different* override of the same virtual in the same file, and
several others' usage was inside a `/* doesn't work */` block
comment, not live code. `/wd4100` **stays in `src/CMakeLists.txt`**
until all 314 are done — 116 remain, the long tail of 1–2-site files.
**Remaining:** `C4100` continues; `C4244`/`C4267` (numeric conversion
/ narrowing, ~4.4k hits) are the real debt — each needs a real look
for actual truncation, not a mechanical pass; still not measured for
Clang/GCC (`baseline.md` TBD).

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

### 16. Pre-floor `#if` guards across `src/arch/` and `src/archutils/` — Windows runtime-version checks DONE
Now that ADR 0003 sets Windows 11 / current-macOS / current-Linux floors,
sweep for `#if`/`#ifdef` guards handling below-floor OSes: `_WIN32_WINNT`
comparisons, `WINVER` checks, `MAC_OS_X_VERSION_MIN_REQUIRED` for old
10.x, XP/9x fallback branches, 32-bit paths, EOL-distro `#ifdef`s.
**Done (2026-09-04), Windows runtime-version checks:**
`ArchHooks_Win32.cpp::BoostPriority()` ran `IsWindowsVersionOrGreater
(Win2000)` at runtime to decide whether `ABOVE_NORMAL_PRIORITY_CLASS`
was usable (always true on the floor) — collapsed to unconditional.
`DSoundHelpers.cpp` did the same for `IsWindowsVistaOrGreater()` gating
`DSBCAPS_TRUEPLAYPOSITION`. Both now-unused `#include "VersionHelpers.h"`
removed (`DirectXHelpers.h` and the two call sites). No preprocessor-level
`#if _WIN32_WINNT`/`WINVER` guards remain in `src/arch`/`src/archutils`
outside `arch_setup.h` (already correct at `0x0A00`, item 13). A few
comments mention Win9x/XP/Win98 as historical context on still-live code
(hardware quirk notes, page-permission differences) — left alone, not
functional gates.
**Not done — 32-bit Windows removal (new, see item 21).**
**Remaining:** macOS `MAC_OS_X_VERSION_MIN_REQUIRED` / old-`.mm` sweep,
Linux EOL-distro `#ifdef`s — both out of scope for now (`AGENTS.md` §3,
non-Windows work needs explicit instruction).

### 21. Drop 32-bit Windows (x86) — ADR 0003 already says so, not executed
ADR [0003](./adr/0003-platform-support-floors.md) (Accepted): "No 32-bit
targets on any platform." Windows 11 (the floor) doesn't even ship a
32-bit edition, so a Windows-R x86 build has nowhere to run — but the
x86 build path is still fully present: `SM_WIN32_ARCH` branches in
`src/CMakeLists.txt`, `StepmaniaCore.cmake`, `tests/CMakeLists.txt`,
`CMake/Modules/FindDirectX.cmake`, and (as of 2026-09-04) the x86 half
of the `build-ffmpeg-win32.yml` artifact / `ffmpeg-w32-19feb712f5`
Release asset (item 7). Found during the item 16 sweep; deliberately
**not executed yet** — flagged to the maintainer first since it
intersects with the just-shipped ffmpeg artifact pipeline.
**Action:** remove the `x86`/Win32 branches from the 4 CMake files
above, then simplify `build-ffmpeg-win32.yml` to only build x64 (drop
the second `./configure`/`make` pass and the `x86/` package dir) —
existing `x64/` Release asset stays valid, no need to re-cut it.
`AGENTS.md` §4 higher-risk change (build-flag/toolchain scope);
Windows build verified before/after.

### 15. `#if 0` dead blocks — two batches DONE, ~7 remain (fragile ones)
First pass (`a2c3d44522`, 2026-09-04): 10 dead blocks across 8 files
(`CodeDetector.cpp`/`.h`, `CourseUtil.cpp`, `NoteDataUtil.cpp` ×3,
`NoteDataWithScoring.cpp`, `NotesWriterSSC.cpp`,
`RageSoundReader_Resample_Good.cpp` ×3, `RageUtil_FileDB.cpp`).
Second pass (`f1d6e6c4ac`, 2026-09-04): `RageUtil_AutoPtr.h` — three
blocks marked `#if 0 // broken VC6` weren't dead code at all, but
working `HiddenPtr<T>` cross-type conversion + the friend declaration
it needs, blocked by a VC6 template bug; **enabled**, not deleted (the
toolchain floor is MSVC v143, nothing left to work around).
`ScreenNameEntry.cpp` — two blocks explicitly labeled "DEBUGGING
STUFF"/"Debugging." (dead ad-hoc dev shortcuts). `StdString.h` — a
generic template superseded by the concrete overloads right after it.
**Left alone both passes, real kept-for-reference or ambiguous cases:**
`NoteData.cpp` (explains why the disabled iterator op is unsafe),
`RageFileManager_ReadAhead.cpp` (explains why `dup()` doesn't work
here), `RageDisplay_GLES2.cpp` (an active `#if 0`/`#else` selector —
the `#else` branch is what's actually compiled), `RandomSample.cpp` /
`ScoreKeeperNormal.cpp` (same, `#if 0`/`#else` or `#if 0`/`#elif 1`/
`#else`), `RageUtil_CachedObject.cpp` (a deliberately-disabled usage
example), `RageSoundReader_MP3.cpp`'s `resync()` (declared in the
header; a comment elsewhere explicitly says "don't use resync(), it's
slow" — deliberate non-use, not abandonment), `RageThreads.cpp`'s
mutex lock-order checker (a complete, working deadlock-detection
feature, just not wired in), `Player.cpp` (author's own "doesn't make
sense" comment, but it's CPU/autoplay scoring — too fragile a hot path
to guess at) and `ScreenEdit.cpp` (looks superseded by the logic that
replaced it, but no explicit disowning comment and it's fragile editor
state-machine code).
**Remaining (~7 sites):** `Player.cpp`, `ScreenEdit.cpp`, plus
whatever's left after re-verifying the rest weren't miscounted. Both
need someone to actually reason through gameplay/editor logic, not a
mechanical read. Out of scope: `archutils/Unix/*` and
`arch/Threads/Threads_Pthreads.*` (non-Windows, `AGENTS.md` §3),
`src/tests/` (unsalvageable, ADR 0006).
**Action:** locate the actual matching `#endif` before judging a
block — a mid-block partial read can look like it ends earlier than
it does (bit us once on `CourseUtil.cpp`, caught by the `WITH_WERROR`
build, not by inspection). Distinguish three shapes before deciding:
plain dead `#if 0 ... #endif` (candidate for removal), an active
`#if 0/#else` or `#if 0/#elif N/#else` selector (never remove — the
other branch is live), and a toolchain-EOL block like the VC6 ones
above (enable, don't delete).

### 18. Logging overhaul — phase 1 DONE, phases 2-4 open
Phase 1 (`c82d0e9058`): bracketed level tags, `Error()` level, no
`/////`, `Char Widths` fixed. `--SelfTest` log 695→467 lines, clean.
**Remaining (ADR [0005](./adr/0005-logging-overhaul.md)):**
- Ph2: `Debug` level, per-category thresholds + `--LogLevel`,
  `Log::Category` enum, `LOG_*` macros capturing `file:line`.
- Ph3: repeat-collapsing (`… (repeated N×)`).
- Ph4: call-site audit per subsystem — `Warn`→`Trace` (expected
  fallback) / `Warn`→`Error` (real failure), add category tags.

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

- **Item 14** — `vcvars64.bat` doesn't wire the Windows SDK
  (2026-09-04). Shipped `Build/dev-env.ps1` (the "ship a project `env`
  helper script" option) instead of repairing the VS install: runs
  `vcvars64.bat` (located via `vswhere.exe`), and if `WindowsSdkDir`
  still comes back empty, wires the newest installed SDK version
  (auto-detected, not hardcoded) into `INCLUDE`/`LIB`/`PATH`. Verified
  end-to-end on the maintainer box: compiled + ran a `<windows.h>`
  program linked against `kernel32.lib`, and `cmake -G Ninja -B build`
  configures clean (previously failed compiler/SDK detection without
  it — the actual documented pain point).
- **Item 19** — OS version detection reported "Windows 8" on Windows 11
  (2026-09-04, `fee51d41e7`). Root cause: the exe manifest had no
  `<compatibility>` `supportedOS` entries, so Windows caps
  `GetVersionEx`/`RtlGetVersion` at 6.2 for any unmanifested app;
  added the Windows 10 GUID (covers 11 too — no separate GUID exists,
  identified only by build ≥ 22000). `DebugInfoHunt.cpp`'s version
  table also had no branch for major version 10 at all. Verified via a
  real `--SelfTest` run: `Logs/info.txt` now reads
  `Windows 10.0 (Win11) build 26200`.
- **Item 8** — unsafe C string ops in the Windows crash/URL/zip paths
  (2026-09-04), scoped to what was in-scope (P1 platform; `archutils/
  Unix/CrashHandler*` untouched per `AGENTS.md` §3). Three commits:
  `GotoURL.cpp` (`d346dacccc`) — the one genuine, reachable overflow:
  a fixed `char[2*MAX_PATH]` `strcat`'d with `sUrl`, which reaches here
  network-supplied via the crash handler's update checker
  (`CrashHandlerChild.cpp`'s `m_sUpdateURL`, parsed from the
  update-check XML response). Confirmed `GotoURL` only ever runs in
  the crash handler's separate child process (`CreateProcess`-spawned,
  not the crashed process's own exception handler) or normal app code,
  so rewrote with `RString` (no fixed capacity to overflow) — safe
  because heap allocation is fine there, unlike `Crash.cpp` itself.
  `Crash.cpp` (`0095f2673d`) — `SpliceProgramPath`, `StartChild`,
  `CrashGetModuleBaseName`: none reachable with an attacker-controlled
  length today, but this file explicitly forbids `malloc`/`new`
  (crash-time), so fixed with bounded `strncpy`-style copies + explicit
  length math instead of a growable string type.
  `CreateZip.cpp` (`448e4412fe`) — `TZip::Add`'s entry-name `_tcscpy`
  into a fixed buffer, rejected instead of overflowed if too long;
  `TZip`/`CreateZip` have no callers anywhere in the current `src/`
  tree, hardened for whenever it's wired up. All three verified against
  a Windows Release build clean under `WITH_WERROR=ON` (`/WX`) +
  `--SelfTest`.
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
