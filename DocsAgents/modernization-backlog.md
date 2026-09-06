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

**Tier 1 is now empty.** Items 1–3 all closed (2026-09-05): the safety
net (smoke + Catch2 harness + characterization coverage of the pure-ish
cores), the MSVC-warning ratchet's mechanical categories, and the 2009
cppcheck leak list. What remains for the warning ratchet is `C4244`/
`C4267`, tracked under Tier 3 / item 2 — it needs a scoping conversation,
not continuous-work unblocking.

### 1. Safety net — DONE 2026-09-05
- **Headless smoke: DONE** (`f7249f3a95`) — `--SelfTest` flag runs full
  engine init and exits 0; wired into Windows CI (`continue-on-error`
  until it's green a few times, then make fatal).
- **Harness scaffold: DONE** (merged `5_1-new` 2026-09-04). ADR
  [0006](./adr/0006-test-harness.md) (Catch2 v3 + `sm_engine` OBJECT
  library); `sm_tests` + first `RageUtil` coverage; CI green on
  Windows/macOS/Linux (see item 17).
- **Unit coverage: DONE 2026-09-05.** Characterization targets, in order:
  `RageUtil` → `RageMath` (2026-09-04) → `TimingData` (2026-09-05 —
  row/measure math + beat<->time conversion via the GAMESTATE/PREFSMAN-
  free `NoOffset` entry points) → `NoteData` (2026-09-05 — tap/hold
  storage, track queries, row traversal; the counting/statistics API
  needs a live GAMESTATE and is out of scope, see item 17) →
  `NoteDataUtil` (2026-09-05 — the pure transform helpers) →
  `NotesLoader*` (2026-09-05 — `tests/test_NotesLoader.cpp`: the
  parse *primitives* — `MsdFile` tokenizer,
  `GetMainAndSubTitlesFromFullTitle`, `SMLoader::RowToBeat`/
  `ParseBPMs`/`ParseStops`/`Process{BPMsAndStops,Delays,
  TimeSignatures,Tickcounts}` on valid input. Full `LoadFromDir`/
  `LoadFromSimfile` needs live `FILEMAN`+`LUA` → stays `--SelfTest`
  smoke; the helpers' `LOG->UserLog` error branches need a live `LOG`.
  A committed simfile corpus is a future add if/when the harness grows
  an engine-bootstrap fixture). **311 assertions / 72 cases.**

### 2. Warnings on but unmeasured / unenforced — ratchet turning, 3 of 5 MSVC categories promoted
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
**Done (2026-09-05):** `C4100` (unreferenced parameter). Real measured
count was **314 unique sites** (not the stale ~1362 raw-line figure,
which double-counts headers across every including TU) spread across
~150 files with no concentration (max 11 in one file) — a wide
mechanical sweep across five commits (`713e58a0f6`, `9844ab3c4b`,
`26b7de158b`, `3636b80690`, `c1d77d662a`), highest-concentration files
first down to the 62 single-site files last. Convention: comment out
the unused parameter name (`Type /* name */`, this codebase's existing
style), except one site genuinely used only under
`#if defined(HAVE_POSIX_FADVISE)` (unset on Windows) —
`[[maybe_unused]]` there instead, since commenting the name would
break the other platform's compile. Several sites looked like false
positives but weren't: verify each site's actual body rather than
assume the pattern — used by a *different* override of the same
virtual in the same file, or usage only inside a `/* doesn't work */`
block comment, or a near-duplicate signature where only one of two
overloads/declarations actually triggered the warning.
`/wd4100` **removed from `src/CMakeLists.txt` permanently** —
promoted to `-Werror` alongside `C4189`/`C4702`. Verified with a full
Release rebuild (not just the touched files) showing zero `C4100`
across all of `src/`, plus `sm_tests` clean under the existing
`WITH_WERROR=ON` config.
**Remaining:** `C4244`/`C4267` (numeric conversion / narrowing, ~4.4k
hits) are the real debt — each needs a real look for actual
truncation, not a mechanical pass; still not measured for Clang/GCC
(`baseline.md` TBD). Not started — larger scope than C4100, needs a
scoping conversation before beginning.

### 3. Stale cppcheck leak list — DONE 2026-09-05, all dismissed
~~`Docs/Devdocs/possible memory leaks.txt` — from 2009. Re-triaged by
hand against the current tree (cppcheck itself isn't installed on the
maintainer box; each site was verified by reading the actual
ownership path instead). All 11 in-scope entries are either false
positives (cppcheck can't model this codebase's manual ownership
idioms — manager-owned resources, sound-reader chains, explicit
refcounting, Actor-tree AddChild/DeleteAllChildren, Lua-script-managed
lifetime, or a deliberate Meyer's-singleton) or already fixed since
2009 (`Font.cpp`'s `pPage`, `RageFileDriverDeflate.cpp`'s `mem` — the
latter via `std::unique_ptr`). `AdjustSync::s_pTimingDataOriginal` no
longer exists (refactored to a `std::vector` value member). Two
referenced files (`PitchDetectionTestUtil.cpp`, `crypto/CryptRSA.cpp`)
are gone entirely. Two entries are non-Windows paths, left unevaluated
per `AGENTS.md` §3 (`archutils/Unix/CrashHandlerChild.cpp`,
`smpackage/ZipArchive/Linux/ZipPlatform.cpp`). Full per-item reasoning
recorded in the file itself and in `log.md`.~~

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
**Done (2026-09-05, `a26c13e00c`):** dropped `_CRT_SECURE_NO_DEPRECATE`
(redundant with the CMake-level `_CRT_SECURE_NO_WARNINGS`) and
`_SCL_SECURE_NO_DEPRECATE` (no-op since VS2017 removed the checked-
iterator feature it suppressed), plus the stale ~30-line VC6/VC2005
comment block. Kept `_CRT_NONSTDC_NO_WARNINGS` (still functionally
relevant — POSIX-name deprecation). `arch_setup.h` itself is now clean.
**Remaining (speculative, no current failure to verify against):**
`src/archutils/Win32/DirectXErrorList.h` — 12 `case` labels
(`0x8007xxxx`) that don't fit signed `HRESULT`; MSVC compiles it fine
(the only compiler actually used on Windows today), clang would
reject it (C++11 narrowing) if clang-cl were ever adopted. Not touched
— no reproducer to verify a fix against, and Windows isn't built with
clang currently. Rewrite the cases as hex literals / `HRESULT(...)` if
and when clang-cl support is actually pursued.

### 17. Pick a unit-test framework + write core characterization tests — phases 1-3 DONE; phase 4 DONE for .sm/.ssc (2026-09-06); .sma/.dwi/.ksf/.bms/.crs + reader-salvage still open
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
**Progress (ADR 0006 phases 2-4):** `RageMath` done (2026-09-04,
`f98d7a489e`, wave/matrix/vector/bezier helpers). `TimingData` done
(2026-09-05, `197ea46f02`) — row/measure math and beat<->time
conversion via the `NoOffset` entry points (the offset-applying
wrappers just add a `GAMESTATE`/`PREFSMAN` read on top, so this
covers the real logic without needing a live engine). `NoteData` done
(2026-09-05, `6d4e6b5aa0`) — tap/hold storage, track queries, row
traversal; its counting/statistics API (`GetNumTapNotes`,
`GetNumMines`, `GetNumHoldNotes`, `GetNumRowsWithSimultaneousTaps`,
...) calls `GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow()`
through `IsTap`/`IsMine`/`IsLift`/`IsFake` and is out of scope for a
GAMESTATE-free test file — only `GetNumTapNotesNoTiming` (the
GAMESTATE-free counterpart) is covered. `NoteDataUtil` done
(2026-09-05, `46001925f5`) — the transform helpers that operate
purely on `NoteData` (and, for `RemoveFakes`, a caller-supplied
`TimingData const&` instead of the global timing data): hold/roll
sub-type conversion, `RemoveSimultaneousNotes`
(`RemoveJumps`/`RemoveHands`) including its removal-order quirk (the
*last* pressed track survives a cutdown, not the first) and that held
tracks count toward the threshold but are never themselves removed,
type filtering (`RemoveMines`/`RemoveLifts`/`RemoveAllTapsOfType`/
`RemoveAllTapsExceptForType`), `ShiftLeft`/`ShiftRight`'s wrap-around
track rotation, `InsertRows`/`DeleteRows`, and
`GetNextEditorPosition`/`GetPrevEditorPosition` treating a hold's tail
as its own stop distinct from its head. 246 assertions / 52 cases
total. `NotesLoader*` done (2026-09-05, `69803e8ca1`,
`tests/test_NotesLoader.cpp`) — the parse *primitives* that turn
simfile text into beats/rows/`TimingData`: the `MsdFile` tokenizer
(`:` param breaks, missing-`;` recovery at the next line's `#`, `//`
comments, `\:` escape under `bUnescape`), `GetMainAndSubTitlesFrom
FullTitle` (five separators, tab-before-`" -"` precedence, the
separator's non-space half stays in the subtitle),
`SMLoader::RowToBeat` (`r`/`R` suffix → ÷rowsPerBeat), `ParseBPMs`/
`ParseStops` (split `,` then `=`, row-suffix conversion — valid input
only), `ProcessBPMsAndStops` (initial BPM at row 0, pre-beat-0 stop →
song offset not segment), `ProcessDelays`/`ProcessTimeSignatures`
(implicit-4/4 back-fill)/`ProcessTickcounts` (clamp to
`ROWS_PER_BEAT`). **Scoped out, deliberately:** full `LoadFromDir`/
`LoadFromSimfile` needs live `FILEMAN`+`LUA`+more → `--SelfTest` smoke
territory per the playbook; the `SMLoader` helpers' error/edge
branches (`"a=b=c"`, zero BPM, zero-length stop, negative beat) all
call `LOG->UserLog()` and `LOG` is null in the harness. 311 assertions
/ 72 cases total. **ADR 0006 phase 2 (pure-ish core characterization)
is complete.**

**Bootstrap fixture DONE (2026-09-06, `tests/EngineTestEnv.{h,cpp}`).**
`EngineTestEnv::Require()` news up `LUA` → `FILEMAN` → `LOG` → `GAMEMAN`
once per `sm_tests` run and mounts `tests/data/` at `/testdata`; a
Catch2 listener tears it down. Retires the "`LOG` is null" scope-out
above.

**Phase 4 for `.sm`/`.ssc` DONE (2026-09-06,
`tests/test_NotesLoaderCorpus.cpp`).** A `GENERATE(from_range(...))`
parse-regression over the **real committed SM5 sample songs**
(`Songs/StepMania 5/{Goin' Under (.sm + .ssc), MechaTribe Assault,
Springtime}`) — **no toy simfiles** (maintainer call 2026-09-06: for
simfiles always use the real ones; a toy only proves the loader
survives input its author understood, not the §5 invariant — see
`tests/data/README.md`). Pins per-song metadata + BPM and per chart
(file order, 39 charts) `StepsType`/`StepsTypeStr`/difficulty/meter/
track count/`GetNumTapNotesNoTiming`, characterization values captured
from a hidden `[.dump]` case. Plus a Goin' Under `.sm`-vs-`.ssc`
cross-format equivalence case. `#NOTES` parsing works now that
`GAMEMAN` is in the fixture; `EngineTestEnv` also mounts the repo
`Songs/` at `/Songs`. Suite **638 assertions / 78 cases** (was
311/72). See `log.md` 2026-09-06 and ADR 0006 phases 3-4.

**`PREFSMAN` added to `EngineTestEnv` (2026-09-06).** Construction is
now `LUA → FILEMAN → LOG → PREFSMAN → GAMEMAN` (teardown reversed —
`~PrefsManager` calls `LUA->UnsetGlobal`). No `.ini` is mounted so every
preference keeps its compiled default; `tests/test_EngineTestEnv.cpp`
pins the bring-up + a few defaults (`m_bQuirksMode` false, `m_bFastLoad`
true, `m_fGlobalOffsetSeconds` -0.008). Suite **651 / 81**. This
unblocks the `LoadFromDir` path for `.dwi`/`.ksf`/`.bms`.

**Still open:**
- `.sma` / `.dwi` / `.ksf` / `.bms` / `.crs`: **no committed sample
  song exists** for any of them (a real `.dwi`, "Nozex", was floated
  but not committed). Each needs one small real song added under
  `Songs/` — then `.sma` goes in the existing `kCorpus`, and
  `.dwi`/`.ksf`/`.bms` get a `LoadFromDir` case (the fixture is ready).
  `.crs` courses reference songs so likely also need `SONGMAN`.
- Salvage `src/tests/test_file_readers.cpp` / `test_audio_readers.cpp`
  (RageFile / audio-reader round-trips) — the `FILEMAN` half is
  available; still needs the round-trip fixtures + cases.

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

- **Item 3** — `Docs/Devdocs/possible memory leaks.txt`, a 2009 cppcheck
  leak list never re-verified (2026-09-05). Re-triaged all 11 in-scope
  entries by hand (cppcheck not installed locally; read each ownership
  path directly). Result: zero live leaks. Most are false positives
  cppcheck's simple checker can't model — manager-owned resources
  (`ActorFrameTexture`), sound-reader chains (`AutoKeysounds`,
  `RageSoundReader_PitchChange`), explicit refcounting
  (`RageSoundReader_ChannelSplit`), the Actor-tree `AddChild`/
  `DeleteAllChildren` idiom (`OptionRow`), Lua-script-managed lifetime
  (`RageFile`), or a deliberate Meyer's-singleton (`RageThreads`'s
  `GetThreadSlotsLock()`). A few were already fixed since 2009
  (`Font.cpp`'s `pPage`; `RageFileDriverDeflate.cpp`'s `mem`, now
  `std::unique_ptr`-owned). `AdjustSync::s_pTimingDataOriginal` no
  longer exists (refactored to a `std::vector` value member).
  `PitchDetectionTestUtil.cpp` and `crypto/CryptRSA.cpp` are gone.
  Two non-Windows entries left unevaluated per `AGENTS.md` §3. Docs-only
  change — no code touched, no rebuild needed. Full reasoning recorded
  in the leak-list file itself.
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
