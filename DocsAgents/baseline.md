---
type: Reference
title: Modernization baseline
description: The measured starting state — warnings, tests, size — that modernization work drives down. Update as passes land.
tags: [baseline, metrics, modernization]
---

# Purpose

The numbers each modernization PR is measured against. When a
[`playbooks/clang-tidy-subsystem-pass`](./playbooks/clang-tidy-subsystem-pass.md)
lands, update the relevant row. This file is the progress bar.

> Rows marked **TBD** need a local build + tool run to fill in — see
> "How to (re)generate". Static facts below are already confirmed.

# Build status

**Green** (2026-09-03), both configs, VS 2022 / v143, cmake 3.31 (bundled):
- **Release** (`--config Release`) → `Program/StepMania.exe` — **this is
  what ships and what the maintainer plays.** Clean `--clean-first`
  rebuild with `-DWITH_WERROR=ON`: exit 0, 0 warnings. ~4 min.
- **Debug** (`--config Debug`) → `Program/StepMania-debug.exe` — dev
  binary: `/Od`, asserts + `/RTC1` on, `DEBUG` macro, ~2.5× larger.

**Verify Release, not just Debug** — `/O2` can surface warnings/UB Debug
hides. `Program/` holds both (CMake names them per config,
`src/CMakeLists.txt` `SM_NAME_*`). Reference green build for the §4 gate.

# Build configuration (confirmed 2026-09-02)

| Item | Value | Source |
|---|---|---|
| CMake minimum | 3.20 | `CMakeLists.txt:1` |
| C++ standard | 17 (`REQUIRED ON`) | `src/CMakeLists.txt:88` |
| GCC/Clang warnings | `-Wall -Wextra -Wno-unused -Wno-unused-parameter -Wno-unknown-pragmas -Werror=type-limits` (+ `-Wno-undefined-var-template`, `-Wno-deprecated-declarations` on Clang) | `src/CMakeLists.txt:126-130` |
| MSVC warnings | `/W4 /wd4100 /wd4189 /wd4244 /wd4267 /wd4702` | `src/CMakeLists.txt:132` |
| `-Werror` | only `type-limits` today. **Policy (ADR 0001 §7):** curated `-Werror` set that grows; a category is promoted once it is zero across `src/`; enforced via `WITH_WERROR` (ON in CI, OFF by default). `WITH_WERROR` not added yet. | `src/CMakeLists.txt:126-132` |
| Platform floors | **Windows 11 x64** min; MSVC v143 / VS2022+. macOS: latest + prior, arm64 primary. Linux: current distros. (ADR 0003.) | ADR 0003 |
| Externals | git submodules; FFmpeg prebuilt for Windows in `extern/ffmpeg-w32/` (~36 MB) — `avcodec-59`, `avformat-59`, `avutil-57`, `swscale-6` ≈ **FFmpeg 5.x**. Origin not documented in-repo. Kept per ADR 0001 §8; replace via submodule build later (backlog 7). | `.gitmodules`, `StepmaniaCore.cmake:179+`, `:201-209` |

# Warnings (MSVC captured 2026-09-03)

Clean rebuild of the `StepMania` target, Debug, VS2022 v143 (14.44.35207),
bundled cmake 3.31.

| Config | Warnings | Notes |
|---|---|---|
| **As shipped** — `/W4` + `/wd4100 /wd4189 /wd4244 /wd4267 /wd4702` | **0** | The build is clean as configured. |
| `/W4`, **suppressions removed** | **2814** (MSBuild dedup) | This is the debt behind the five `/wd` flags. |
| Clang `-Wall -Wextra` | **TBD** | needs a clang build |
| GCC `-Wall -Wextra` | **TBD** | Linux, P3 |

Debt breakdown (raw hit lines, incl. header dupes across TUs):

| Code | Hits | Meaning | Fix shape |
|---|---|---|---|
| `C4244` | 2736 | conversion, possible loss of data (`double`→`float`, `int`→`char`…) | `static_cast`, review each for real truncation |
| `C4267` | 1728 | `size_t` → smaller type | same; often `int` loop vars that should be `size_t` |
| `C4100` | 1362 | unreferenced formal parameter | drop the name / `[[maybe_unused]]` |
| `C4189` | 26 | local var init but unused | delete |
| `C4702` | 10 | unreachable code | delete / restructure |

C4244+C4267 (~4.4k raw / most of the 2814) are the bulk and the only ones
that can hide real bugs (silent truncation) — highest-value to burn down.
Per-subsystem counts: TBD as passes run
([`playbooks/clang-tidy-subsystem-pass.md`](./playbooks/clang-tidy-subsystem-pass.md)).

# clang-tidy (captured 2026-09-03)

Repo-root `.clang-tidy` starter set, run over the 421 in-scope `src/`
`.cpp` (clang-tidy 23.1.0, `-p build-tidy`, `--extra-arg-before=/Y-`).
Counts are raw hit **lines** — a finding in a shared header is counted
once per including TU, so unique counts (esp. `use-override`,
`macro-parentheses`) are lower.

| Check | Hits | Nature |
|---|---:|---|
| `readability-container-size-empty` | 463 | `.size()`→`.empty()`, `==""`→`.empty()`. Autofix, safe. |
| `modernize-use-override` | 367 | add `override`. Autofix, safe. |
| `bugprone-macro-parentheses` | 197 | paren-wrap macro args. Autofix, low-risk; sometimes catches real precedence bugs. |
| `modernize-use-nullptr` | 43 | `NULL`/`0`→`nullptr`. Autofix; review variadic calls. |
| `modernize-use-equals-default` | 41 | `{}` body → `= default`. Autofix. |
| `modernize-use-bool-literals` | 24 | `0/1`→`false/true` for bool. Autofix. |
| `readability-redundant-member-init` | 22 | drop `m_x(T())`. Autofix. |
| `bugprone-integer-division` | 14 | `int/int` assigned to float — **look at each**, likely real. |
| `bugprone-suspicious-string-compare` | 11 | `strcmp` misuse — **look at each**. |
| `modernize-redundant-void-arg` | 3 | `(void)`→`()`. Autofix. |
| `modernize-make-unique` | 2 | `new`→`make_unique`. |
| **Total** | **1187** | |

The top 3 (~1030) are near-mechanical autofixes → obvious first
`clang-tidy-subsystem-pass` targets. `bugprone-integer-division` and
`bugprone-suspicious-string-compare` (25) want human eyes — potential real
bugs.

**Parse errors:** 12, all from `src/archutils/Win32/DirectXErrorList.h` —
`case` labels like `0x8007000B` used in a `switch` on signed `HRESULT`
(clang C++11-narrowing strictness; MSVC allows it). Non-conforming but
isolated; write the cases as hex/`HRESULT(...)`. Backlog note under item 13.

Per-subsystem breakdown as passes run:

| Subsystem | check | before | after | commit |
|---|---|---:|---:|---|
| rage (`src/Rage*.cpp`) | `readability-container-size-empty` | ~98 | 0 | `10a1ba54ec` |
| rage (`src/Rage*`) | `modernize-use-override` | ~65 | 0 | `689e35a486` |

# Tests

- **Headless smoke test: EXISTS** as of `f7249f3a95` (2026-09-03).
  `stepmania --SelfTest --VideoRenderers=null --SoundDrivers=null` runs
  full engine init and exits 0. Verified locally (~11s). Wired into the
  Windows CI job (`continue-on-error` for now). This is the `AGENTS.md`
  §4 smoke test.
- `src/tests/`: 7 standalone `test_*.cpp` from ~2004-06 — **Unix/Apple
  only, need uncommitted 30 MB test data, `#error` without altivec/SSE,
  full of `#if 0`**. Not wireable as-is; they're a rewrite. Real unit
  coverage needs a decision (Catch2 / doctest — backlog item 17) and new
  tests on the pure cores. Salvage the *intent* (timing data, file
  readers) into new tests where still relevant.
- CI: build on 4 platforms + `xmllint` Lua-doc validation + the Windows
  headless smoke.

**Smoke-test plan (recon 2026-09-02).** No headless / boot-and-exit mode
exists. `CommandLineActions::Handle()` (`StepMania.cpp:960`) runs *after*
manager init (incl. `DISPLAY`); `--ExportLuaInformation` and `--version`
then `exit(0)`. Renderer is chosen from `PREFSMAN->m_sVideoRenderers`;
`"null"` → `RageDisplay_Null` (`StepMania.cpp:661`); prefs are
override-able as `--<PrefName>=<value>`. So the smallest smoke test is a
new `--SelfTest` (or `--ExitAfterInit`) flag that: forces null
display/sound drivers, completes init, optionally spins the game loop N
frames, prints a success marker, `exit(0)`. Small first code change;
then a CI job runs it. Feeds the `AGENTS.md` §4 gate ("headless smoke
test passes").

# Known issues carried from before the fork

- `Docs/Devdocs/possible memory leaks.txt` — cppcheck run from **2009**,
  never cleared. Likely-still-live entries: `ActorFrameTexture::m_pRenderTarget`,
  `AdjustSync::s_pTimingDataOriginal`, `AutoKeysounds::m_pSharedSound`,
  `LifeMeterTime::m_pStream`, `MusicWheelItem::m_pTextSectionCount`,
  `OptionRow::m_textTitle`, `Font.cpp` `pPage`, `RageFileDriverDeflate.cpp`.
- Dead CI: `.travis.yml` (travis-ci.org shut 2021) + stale `README.md`
  badges. GitHub Actions `ci.yml` is the live one.
- Dead code: `src/irc/` notifier targets `irc.freenode.net` (defunct 2021).
- Stale docs: `Build/README.md` states "CMake min 2.8.12" (actual: 3.20).
- Unsafe C string ops concentrated in crash handlers
  (`archutils/*/Crash*`, `CrashHandler*`), `CreateZip.cpp`,
  `archutils/Win32/GotoURL.cpp`.
- Prebuilt, unpatchable FFmpeg binaries in the repo for the Windows build.

Full ranked list: [`modernization-backlog.md`](./modernization-backlog.md).

# Size snapshot (LOC, confirmed 2026-09-02)

`src/*.cpp` total ≈ 187,800. Largest TUs (refactor risk = high):

| File | LOC |
|---|---|
| `ScreenEdit.cpp` | 6596 |
| `GameManager.cpp` | 3614 |
| `Player.cpp` | 3567 |
| `GameState.cpp` | 3523 |
| `ScreenGameplay.cpp` | 3381 |
| `NoteDataUtil.cpp` | 3379 |
| `Profile.cpp` | 2897 |
| `RageDisplay_OGL.cpp` | 2856 |
| `RageUtil.cpp` | 2818 |
| `PlayerOptions.cpp` | 2664 |

Cross-cutting: `RString` in 723 files / ~8,429 uses; `GAMESTATE->` at
~2,104 call sites.

# How to (re)generate

## Environment gotchas on the maintainer box (2026-09-03)

- Two `cmake` on the machine: standalone **4.3.3** (`C:\Program Files\CMake`)
  and VS-bundled **3.31** (`…\BuildTools\…\CommonExtensions\…\CMake\bin`).
  The `build/` dir was configured with **3.31** — reconfiguring it with
  4.3.3 breaks (mixed module versions). Use the bundled 3.31 for `build/`.
- `vcvars64.bat` does **not** set the Windows SDK env (backlog 14). For
  Ninja / command-line builds, set by hand:
  - `INCLUDE` += `<Kit>\Include\10.0.26100.0\{ucrt,um,shared,winrt,cppwinrt}`
  - `LIB` += `<Kit>\Lib\10.0.26100.0\{ucrt,um}\x64`
  - `PATH` += `<MSVC>\bin\Hostx64\x64`, `<Kit>\bin\10.0.26100.0\x64`
  - `<Kit>` = `C:\Program Files (x86)\Windows Kits\10`;
    `<MSVC>` = `…\BuildTools\VC\Tools\MSVC\14.44.35207`
- `pwsh` at `C:\Program Files\PowerShell\7\pwsh.exe`. `cmd`/`python`/`nasm`
  are **not** on PATH — use full paths. No `python` → `run-clang-tidy`
  (`.py`) can't run; drive `clang-tidy.exe` directly.

## Warnings (MSVC)

1. Reconfigure with bundled cmake 3.31 if needed: `"<vs-cmake>" -B build`.
2. Clean rebuild: `"<vs-cmake>" --build build --target StepMania --config Debug --clean-first -- -verbosity:normal -maxcpucount`
   (Git Bash mangles `/v:n` → use `-verbosity:normal`; set
   `MSYS_NO_PATHCONV=1`). Minimal verbosity hides warnings.
3. `grep -oE "warning C[0-9]{4}" log | sort | uniq -c`. MSBuild's
   `N Warning(s)` summary line is the deduped total.
4. For the "debt behind suppressions": drop
   `/wd4100 /wd4189 /wd4244 /wd4267 /wd4702` from `src/CMakeLists.txt:132`,
   rebuild, count, `git checkout src/CMakeLists.txt`, reconfigure back.

## clang-tidy

1. Configure a Ninja tree (VS generator can't emit compile_commands):
   in the manual MSVC env above,
   `"<vs-cmake>" -S . -B build-tidy -G Ninja -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
   (`ninja.exe` on PATH). Configure only — no build needed.
2. `clang-tidy.exe -p build-tidy --quiet --extra-arg-before=/Y- <file>`.
   **`/Y-` is required** — the compile commands carry `/Yuglobal.h` and
   the PCH is never built, which otherwise fails every file.
3. Repo-wide: filter `compile_commands.json` `.file` to `src\` minus
   `libtomcrypt|libtommath|smpackage|extern|Texture Font Generator`
   (~422 files), run in parallel (`ForEach-Object -Parallel`), aggregate
   the `[check-name]` tags. Script: `scratchpad/run_tidy.ps1`.

## Tests / smoke

Once a test target + `--SelfTest` flag exist, record pass/fail + a
headless run here.

Then update the tables above and add a dated note to [`log.md`](./log.md).
