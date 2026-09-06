# Knowledge Base Update Log

## 2026-09-02

* **Initialization**: Created the `DocsAgents/` OKF bundle. Added
  [`index.md`](./index.md) (repo routing table), [`build.md`](./build.md),
  [`conventions.md`](./conventions.md), [`architecture.md`](./architecture.md).
* **Initialization**: Added [`subsystems/`](./subsystems/index.md) with
  concepts for rage, arch, input, actors, screens, singletons,
  data-structures, simfile-formats, gameplay, lua, theming, noteskins.
* **Creation**: Added repo-root `AGENTS.md` (entry point, language +
  fork-hygiene rules) and git-ignored `CLAUDE.md` (points to `AGENTS.md`).
* **Update**: Moved `spec.md` from repo root to `DocsAgents/spec.md`.
* **Update**: Added `/CLAUDE.md` to `.gitignore`.
* **Update**: Recorded the fork's mission (modernization + shipping
  releases; upstream inactive ~11 years) in [`index.md`](./index.md).
* **Update**: Added `AGENTS.md` §3 (platform priority: Windows > macOS >
  Linux; no non-Windows work unless instructed) and §4 (continuous
  modernization, but large changes halt for maintainer manual
  verification). Renumbered later sections. Mirrored the summary into
  [`conventions.md`](./conventions.md).

* **Creation**: Added [`playbooks/`](./playbooks/index.md) with
  `_template.md` and four seed playbooks: `add-ssc-tag`,
  `expose-lua-api`, `add-screen`, `clang-tidy-subsystem-pass`. Seeds are
  built from code recon, not yet executed end-to-end — verify anchors
  before relying on them, then update the playbook's History.
* **Update**: Added `AGENTS.md` §6.1 (deposit rule: read playbooks/
  subsystem docs before a task; after a corrected task leave a trace —
  playbook, gotcha, or ADR; manual-verification findings go the same
  way). Linked playbooks from [`index.md`](./index.md).

* **Creation**: Added three more playbooks — `add-preference`,
  `migrate-rstring`, `split-god-object` — and reorganized
  [`playbooks/index.md`](./playbooks/index.md) (feature vs modernization).
* **Creation**: Added [`adr/`](./adr/index.md) with
  [`0001-toolchain-target.md`](./adr/0001-toolchain-target.md) — **status
  Proposed**. Settled: Windows>macOS>Linux, CMake 3.20, stay C++17 for
  now, incremental per-subsystem, opportunistic RString retirement,
  first cleanup cut (travis + irc). Open questions A–F await maintainer
  decision.
* **Creation**: Added [`baseline.md`](./baseline.md) (build config
  confirmed; warning/tidy/test counts marked TBD pending a local run) and
  [`modernization-backlog.md`](./modernization-backlog.md) (ranked Tier
  1–4 from the 2026-09-02 source sweep).
* **Creation**: Added repo-root `.clang-tidy` — fork-local, conservative
  starter check set; not run by the build. Referenced by the clang-tidy
  playbook and ADR 0001.
* **Update**: Routing table in [`index.md`](./index.md) now points to the
  backlog, baseline, and ADRs.

* **Decision**: Maintainer dropped the "stay mergeable with upstream"
  constraint. StepMania-R is now an **independent project** — recorded as
  ADR [`0002`](./adr/0002-independent-project.md) (**Accepted**).
  Rewrote `AGENTS.md` §2 (Fork hygiene → Repository conventions);
  reworded ADR 0001 Settled #4/#5 and Consequences to stand on
  safety/review grounds; `Docs/` is now editable (updated
  [`index.md`](./index.md), `conventions.md`, `modernization-backlog.md`
  item 6, playbooks `add-ssc-tag` / `expose-lua-api` /
  `clang-tidy-subsystem-pass`, `subsystems/noteskins` +
  `simfile-formats`). The §4 verification gate and Windows-first priority
  are unchanged.

* **Update**: Added `AGENTS.md` §5 — **Simfile & content compatibility
  (MUST)**: every supported format (`.sm` `.ssc` `.sma` `.dwi` `.bms`
  `.ksf` `.crs` `.lrc`) keeps loading with no parse regression; `.ssc`
  write output stays readable by prior SM 5.x; parse-path changes are §4
  + corpus-tested. Renumbered KB sections (was §5/§6/§6.1 → §6/§7/§7.1);
  updated refs in `index.md`, `conventions.md`, `adr/0002`. Callout added
  to [`subsystems/simfile-formats.md`](./subsystems/simfile-formats.md).

* **Decision**: Resolved ADR 0001's open questions → **Status: Accepted**.
  A → curated `-Werror` set that grows (`WITH_WERROR`, ON in CI only),
  promote a category when it hits zero (new §7). D → keep
  `extern/ffmpeg-w32/` now (load-bearing on Windows), replace via
  submodule build later, record versions in `baseline.md` (§8). E →
  **Windows 10 x64 min, MSVC v143 / VS2022** (§9). F → C++20 after the
  baseline, own ADR (§10). B & C (drop D3D9 / GLES2) → **deferred to ADR
  0003 (renderer strategy)**; both frozen until then. Propagated to
  `adr/index.md`, `baseline.md`, `build.md`, `modernization-backlog.md`
  (items 2, 7), `playbooks/clang-tidy-subsystem-pass.md`.

* **Build/tooling session (2026-09-03).** Confirmed the Windows build is
  **green** (`baseline.md` → Build status). Captured MSVC warning baseline:
  **0 as shipped**, **2814 with the 5 `/wd` suppressions removed** (mostly
  C4244/C4267 numeric conversions). Downloaded ninja 1.13.2 + LLVM 23.1.0
  to the scratchpad; generated `build-tidy/compile_commands.json` (Ninja
  gen + hand-built MSVC/SDK env — `vcvars64.bat` doesn't wire the SDK,
  backlog 14). Ran the `.clang-tidy` starter set over 421 `src/` files:
  **1187 findings**, top 3 (`container-size-empty` 463, `use-override`
  367, `macro-parentheses` 197) are mechanical autofixes. Full recipe +
  numbers in `baseline.md`. Local uncommitted change:
  `src/archutils/Win32/arch_setup.h` −2 dead `#define`s (isnan/isfinite),
  MSVC re-verified green — **awaiting maintainer sign-off** as the first
  §4 change. New backlog items 13 (`arch_setup.h` legacy),
  14 (`vcvars64.bat`).

* **Workflow change (2026-09-03):** maintainer dropped the blocking
  manual-verification gate. New `AGENTS.md` §4 — **commit and push
  autonomously**, don't wait for approval; keep commits small and
  single-purpose; the maintainer reviews async and `git revert`s
  mistakes. Propagated to `conventions.md`, `adr/0002`, all playbooks,
  and memory (`modernization-process`, `work-style-conciseness`).
* **First commits pushed to `origin 5_1-new` (2026-09-03):**
  `8af333de6c` DocsAgents bundle + `AGENTS.md` + `.clang-tidy`;
  `0054e61d95` README; `37e6766d5e` drop dead `isnan`/`isfinite` macros;
  `e065f69c8b` remove dead Travis/AppVeyor; `718d3b3ec1` remove orphaned
  `src/irc/`. Backlog items 4, 5 closed; 13 partial. New items 14
  (`vcvars64` SDK), 15 (`#if 0` sweep).

* **Decision (2026-09-03):** platform floors raised. New ADR
  [`0003`](./adr/0003-platform-support-floors.md) (**Accepted**) —
  **Windows 11 x64**, latest macOS + prior, current Linux distros;
  no 32-bit. Supersedes ADR 0001 §9 (was Win10). Below-floor support
  code (Win7/8/10, XP/9x, `_WIN32_WINNT` < `0x0A00`, old-macOS `.mm`,
  EOL-distro shims) is now delete-on-sight (`AGENTS.md` §3.2a). Renderer
  ADR renumbered 0003 → **0004**. Propagated to `adr/0001` §9 + §deferred,
  `adr/index.md`, `build.md`, `baseline.md`, `AGENTS.md` §3;
  `modernization-backlog.md` item 13 reframed, new item 16 (pre-floor
  `#if` sweep).

* **Step-2 push (2026-09-03), all on `origin 5_1-new`:**
  `f533ec3fb1` popn noteskin (intentional fork content) ·
  `a48f4bdacf` `WITH_WERROR` option + Windows-CI `-DWITH_WERROR=ON`
  (MSVC build verified clean at `/WX`) ·
  `10a1ba54ec` rage × `container-size-empty` (~98→0) ·
  `689e35a486` rage × `modernize-use-override` (~65→0) ·
  `f7249f3a95` **`--SelfTest` headless smoke** — runs full init, exits 0
  (verified locally ~11s), wired into Windows CI (`continue-on-error`).
  Pilot found and fixed the tidy flow (`FormatStyle: none`, no
  `--fix-errors`, exclude Linux-only). `src/tests/` confirmed
  unsalvageable → backlog 17 (framework decision). Backlog 1 smoke part
  closed.

* **Logging overhaul (2026-09-03), `c82d0e9058`** + ADR
  [`0005`](./adr/0005-logging-overhaul.md) (**Accepted**, phase 1).
  `RageLog::Write` now stamps every line with a bracketed level tag
  (`[TRACE] [INFO] [WARN] [ERROR]`), dropped the `/////` warning frame
  and `WARNING:` prefix, added `RageLog::Error()`. `IniFile::RenameKey`
  no longer warns on an absent source key (the `Char Widths` spam).
  `--SelfTest` `log.txt`: 695 → 467 lines, **0 `[WARN]`/`[ERROR]` on a
  clean boot**. Backlog: item 18 (ph2-4 open), item 19 (OS reports
  "Win8" — manifest). `conventions.md` logging section updated.

* **Process fix (2026-09-03).** This session verified code changes
  against **Debug only**; `Program/StepMania.exe` (Release) was stale
  from Sept 2 — the maintainer plays Release, so saw none of the changes.
  Did a clean Release `--clean-first` rebuild with `-DWITH_WERROR=ON`:
  **green, 0 warnings** — so all session commits are Release-clean
  retroactively. Fixed CI (all jobs now `-DCMAKE_BUILD_TYPE=Release` +
  `--config Release`; Windows smoke runs `StepMania.exe`) and `AGENTS.md`
  §4 + `baseline.md` ("verify Release, not just Debug"). Not committed as
  a separate hash yet — folded into the next push.

* **Executables renamed (2026-09-03, `6d62e88f3f`):**
  `StepMania.exe` → `StepMania-R.exe`, `StepMania-debug.exe` →
  `StepMania-R_debug.exe` (Linux: `stepmania` → `stepmania-r`). Source:
  `src/CMakeLists.txt` `SM_NAME_*`. CMake target/project name stays
  `StepMania`. Updated `stepmania.nsi`, `stepmania.desktop`, `.gitignore`,
  CI smoke, `AGENTS.md` §4, `baseline.md`. Both configs rebuilt clean
  with `/WX`.

* **All game types enabled (2026-09-03, `229d0769d5`).** Uncommented
  KB7/Ez2/Para/DS3DDX/Beat/Maniax/Popn/Kickbox in `GameManager.cpp`
  `g_Games[]` (upstream had them off). A game still needs a NoteSkin to
  show in Select Game, so effective additions: kb7, para, beat, popn,
  kickbox. Documented the archaic hand-coded game-type mechanism as
  backlog **item 20** (data-driven registry, keep `#STEPSTYPE` contract;
  ADR-worthy) — also noted in `subsystems/data-structures.md`. Theme
  gaps for the new games (per-game style/difficulty assets) are a
  play-test follow-up.

* **North star recorded (2026-09-03).** Added an opening section to
  `AGENTS.md` — "The point of all this: de-hard-code the engine" — with
  how to recognise hard-coding, what it resolves to (metric / data file /
  runtime registry / option / plugin point), guardrails (on-disk
  contracts are NOT hard-coding to remove; tiebreaker not mandate), and
  how to apply it (small → do it; large → backlog item + maybe ADR).
  Mirrored in `index.md` and the `fork-mission` memory.

* **Stale build docs rewritten (2026-09-03).** Backlog **item 6**.
  `Build/README.md` and `Build/INSTALL.md` were pre-fork text: "CMake min
  2.8.12 / latest 3.3.0-rc3", `cmake -G {gen} .. && cmake ..` in-source
  flow, Windows XP `-T "v###_xp"` toolset, `Stepmania 5` /
  `stepmania-5.1` install paths, `StepMania.app`. Replaced with current
  reality: CMake ≥ 3.20, C++17, VS 2022, `cmake -B build` +
  `cmake --build build` (matches CI), Windows 11 floor (ADR 0003), no
  install step for dev/play (runs from repo root), `StepMania-R`
  executable names. Also fixed `build.md` prereqs (2.8.12 → 3.20) and the
  `Build/StepMania.sln` → `build/StepMania.sln` path. Root `README.md`
  Travis badges were already gone (now a one-line tagline).

* **Test harness decided — ADR
  [0006](./adr/0006-test-harness.md) (2026-09-03).** Backlog **item 17**
  resolved: framework is **Catch2 v3** (amalgamated, vendored at
  `extern/Catch2/`, pinned v3.16.0) over doctest / GoogleTest — decided
  on corpus-regression ergonomics (`GENERATE(from_range)`) and `WithinULP`
  float matchers for `TimingData`. Build approach: `src/` becomes an
  **OBJECT library** (`sm_engine`) that both `${SM_EXE_NAME}` and a new
  `sm_tests` target consume; `Main.cpp` stays exe-only so its `main`
  never collides with Catch2's. Gated behind `WITH_TESTS` (default OFF,
  CI-on). This commit: ADR + vendored Catch2 files +
  `extern/CMakeProject-catch2.cmake` (not yet `include()`d — inert).

* **Test harness scaffold — branch `feature/test-harness` (2026-09-03).**
  The ADR-0006 §4 large change:
  - `CMake/DefineOptions.cmake` — `option(WITH_TESTS OFF)`.
  - `src/CMakeLists.txt` — when `WITH_TESTS`, `src/` builds as
    `add_library(sm_engine OBJECT …)` and the exe links it + the platform
    entry source (`Main.cpp`, or `archutils/Darwin/SMMain.mm` on Apple —
    both pulled out of the engine list so their `main()` never collides
    with Catch2's); otherwise a new `SM_ENGINE_TGT` var just aliases the
    exe and the file is unchanged. Engine compile defs flipped
    `PRIVATE`→`PUBLIC` (no-op
    on a leaf exe; needed so `sm_tests` inherits them); link libs +
    include dirs moved to `sm_engine PUBLIC`; output-name /
    RUNTIME_OUTPUT_DIRECTORY / link-flags / `mapconv` POST_BUILD /
    `install()` stay on the exe.
  - `extern/CMakeLists.txt` — `include(CMakeProject-catch2.cmake)` under
    `if(WITH_TESTS)`. `CMakeLists.txt` — `if(WITH_TESTS) enable_testing();
    add_subdirectory(tests)`.
  - `tests/CMakeLists.txt` + `tests/test_RageUtil.cpp` (8 `TEST_CASE`s
    pinning `Trim`/`TrimLeft`/`TrimRight`/`GetExtension`/
    `GetFileNameWithoutExtension`/`SetExtension`/`Basename`/`BinaryToHex`/
    `ssprintf`, quirks included).
  - `.github/workflows/ci.yml` — `windows-tests` / `ubuntu-tests` /
    `macos-tests` (arm64) jobs, `-DWITH_TESTS=ON` Debug build + `ctest`.
  - `playbooks/add-characterization-test.md`.
  **Verified locally on Windows** (VS 2022 gen, bundled cmake 3.31):
  `WITH_TESTS=ON` Debug + `/WX` fully compiles (`sm_engine` OBJECT lib +
  `Catch2` + `sm_tests.exe`), `sm_tests.exe` → 27 assertions / 8 cases
  pass, `ctest` 100%; `WITH_TESTS=OFF` Release + `/WX` still builds
  `StepMania-R.exe` clean and configure emits no new targets. Non-Windows
  `WITH_TESTS` paths (Apple `SMMain.mm` split, Linux) are
  **configure-checked only** — maintainer verifies on an M1 + WSL/Linux;
  then `ubuntu-tests` / `macos-tests` + the Actions run are the §4 gate.
  Next phases (ADR 0006): `RageMath` / `TimingData` / `NoteData`, then a
  committed simfile corpus via `GENERATE(from_range(...))`.

### Notes for future maintainers

* Subsystem docs are a first pass built from `src/CMakeData-*.cmake`
  groupings and code reconnaissance. Verify specifics against the code
  before relying on them; correct in place and log the change here.
* Seed playbooks are built from code recon, **not executed end-to-end**.
  First executor of each verifies line anchors / steps and updates that
  doc's `History` section.
* ADR 0001 is **Accepted**. ADR 0003 (renderer strategy) is **not
  written** — needed before touching D3D9 / GLES2 / the GL backend.
* `baseline.md` warning/tidy/test counts are **TBD** until someone runs a
  local build + clang-tidy (Windows). That is the concrete next action
  for "step 2", along with adding `WITH_WERROR` and wiring a test target.
* Still not written: a generated symbol/ctags index; playbooks
  `fix-memory-leak`, `harden-c-string` (both listed in
  `playbooks/index.md` → Wanted).

## 2026-09-04

* **Gotcha found + fixed** (`c29368cb40`, branch `feature/test-harness`):
  `ubuntu-tests` CI job (`sm_tests` on Linux) failed to link —
  `undefined reference to LoadingWindow_Gtk::LoadingWindow_Gtk()`.
  Root cause: `src/CMakeData-gtk.cmake` built `LoadingWindow_Gtk.cpp` as
  its own `OBJECT` library (`LoadingWindowGtk`), linked into `sm_engine`
  via `target_link_libraries(... PUBLIC ...)`. CMake does not propagate
  an OBJECT library's objects transitively through *another* OBJECT
  library — it only pulls them into a "real" binary target. That holds
  for the normal exe (`WITH_TESTS=OFF`, `SM_ENGINE_TGT` = the exe itself)
  but not for `sm_tests` (`WITH_TESTS=ON`, `SM_ENGINE_TGT` = `sm_engine`,
  itself an OBJECT library). Windows/macOS never hit this because their
  loading-window sources are plain files in `SMDATA_ALL_ARCH_SRC`, not a
  separate OBJECT-library target — only the Linux/GTK path is shaped this
  way. **Fix**: changed `LoadingWindowGtk` from `OBJECT` to `STATIC` —
  static libraries resolve normally through any number of
  `target_link_libraries()` hops. Linux-only change, no effect on the
  shipped exe (already linked correctly) or on Windows/macOS. **Verified
  green** on all 8 `feature/test-harness` CI jobs (Windows/macOS/Linux ×
  plain build + `sm_tests`, plus the Lua.xml validator) — run
  [33894450910](https://github.com/RieLoveChan/StepMania-R/actions/runs/33894450910).
  See [`adr/0006-test-harness.md`](./adr/0006-test-harness.md).

* **`RageMath` characterization coverage** (`f98d7a489e`) — second test
  file (ADR 0006 phase 2, after `RageUtil`): wave/matrix/vector/bezier
  helpers, pins the `RageSquare(0)` hack and the `RageMatrixMultiply`
  reversed-argument (`pOut = pB * pA`) quirk. 94 assertions / 19 cases
  green across all 3 platforms.

* **Backlog item 7 closed — FFmpeg on Windows now CI-built, not
  committed** (2026-09-04, phased per an approved plan). Replaced the
  36 MB `extern/ffmpeg-w32/` blob (unknown provenance) with a
  reproducible pipeline:
  - `.github/workflows/build-ffmpeg-win32.yml` (`0be3d9f91c`,
    `workflow_dispatch`) cross-compiles `extern/ffmpeg` (pinned
    `19feb712f5`) with mingw-w64 on `ubuntu-latest` — no Windows runner
    needed for the FFmpeg build itself — using the exact recipe recorded
    in the old blob's own `README.txt`. `gendef` + `llvm-lib` turn each
    built DLL's export table into an MSVC-compatible `.lib` (GNU
    dlltool's default `.a` isn't MSVC-linkable). Deviates from the
    original recipe by dropping `--enable-bzlib`/`-zlib` (no Ubuntu
    mingw-w64 package for either; not needed for StepMania's codec
    paths).
  - Published as GitHub Release `ffmpeg-w32-19feb712f5` — this repo's
    first-ever Release object, used purely as a binary artifact store
    (Actions artifacts expire at 90 days, which would silently break a
    fresh clone months later).
  - `CMake/SetupFfmpegWin32.cmake` (`4014f275e9`) downloads +
    SHA256-verifies that asset into `extern/ffmpeg-w32-prebuilt/` at
    configure time, skipping re-download once the hash already matches
    (offline on repeat configures). `StepmaniaCore.cmake`,
    `src/CMakeLists.txt`, `tests/CMakeLists.txt` now point at
    `SM_FFMPEG_W32_DIR` instead of the old hardcoded path.
  - **Gotcha caught by real end-to-end testing, not just inspection:**
    the first packaged `.lib`s were named after the DLL's
    SONAME-versioned basename (`avcodec-59.lib`); the linker and
    `find_library(NAMES "avcodec" ...)` expect the unversioned name
    (`avcodec.lib`) — this only surfaced when actually linking
    `StepMania-R.exe` against the artifact, not from inspecting the zip.
    Fixed in the workflow and by re-uploading a corrected Release asset.
  - Verified locally (fresh `build/` + `build-tests/`, artifact directory
    deleted first to force a real network download against the published
    Release): Release build links + `--SelfTest` exits 0; `WITH_TESTS=ON`
    Debug build links `sm_tests.exe` clean. **CI: all 8 jobs green**,
    including the Windows runner independently downloading the same
    Release asset.

* **Backlog item 8 closed — unsafe C string ops, Windows crash/URL/zip
  paths** (2026-09-04). `GotoURL.cpp` (`d346dacccc`) had a real,
  reachable stack buffer overflow: a fixed `char[2*MAX_PATH]` built a
  fallback shell-open command via `strcat(szPos, sUrl)` with no bound
  on `sUrl`'s length, and `sUrl` reaches `GotoURL()` network-supplied
  through the crash handler's update checker
  (`CrashHandlerChild.cpp`'s `m_sUpdateURL`, parsed straight from the
  update-check XML response `<UpdateAvailable>`). Traced `GotoURL`'s
  callers to confirm it never runs inside the crashed process's own
  exception handler — only in the crash handler's separate
  `CreateProcess`-spawned child, or normal application code — so heap
  allocation is safe there; rewrote with `RString`, preserving the
  original (already slightly odd) control flow bug-for-bug, including
  the "no `%1` placeholder found" branch, whose `strcat`-from-mid-buffer
  trick turned out to always append at the true end of the string
  either way (traced through `strcat`'s scan-to-null-terminator
  semantics by hand to confirm).
  `Crash.cpp` (`0095f2673d`) — three more `strcpy`/`strcat` sites,
  none reachable with an attacker-controlled length today, but this
  file explicitly forbids `malloc`/`new` (runs at crash time), so
  fixed with bounded `strncpy`-style copies + explicit length math
  (or, for the one `szBuf` case, just sizing the buffer for its own
  known-fixed suffix) rather than a growable string type.
  `CreateZip.cpp` (`448e4412fe`) — `TZip::Add`'s entry-name copy,
  rejected instead of overflowed if too long; confirmed `TZip`/
  `CreateZip` have zero callers anywhere in the current `src/` tree
  (dead code today, but compiled and named in the backlog).
  Left alone, deliberately: two more `strcpy`/`strcat` sites in
  `Crash.cpp` (`m_CrashReason` from the fixed `exceptions[]` table,
  and appending a fixed literal) copy only compile-time-bounded
  literals into an 8 KB buffer — safe by construction, not what the
  backlog flagged, fixing them would be pure churn.

* **Backlog item 19 closed — OS version detection** (`fee51d41e7`,
  2026-09-04). The exe manifest declared no `<compatibility>`
  `supportedOS` GUIDs, so Windows capped `GetVersionEx` at 6.2 (Win8)
  regardless of the real OS; added the Windows 10 GUID (Windows 11 has
  no separate one — same NT 10.0, identified only by build ≥ 22000)
  plus a matching branch in `DebugInfoHunt.cpp`'s version table (which
  had nothing past major version 6). Hit and fixed an XML gotcha along
  the way: a bare `--` inside an XML comment body (not at the `-->`
  terminator) makes `mt.exe` fail to parse the manifest with an opaque
  "general error c1010070" — LNK1327 at link time, not a compile
  error. Verified via a real `--SelfTest` run:
  `Logs/info.txt` → `Windows 10.0 (Win11) build 26200`.

* **Backlog item 15, first batch** (`a2c3d44522`, 2026-09-04): 10 of
  ~23 remaining dead `#if 0` blocks removed across 8 files. Real
  gotcha: `CourseUtil.cpp`'s block extended past where a mid-block
  read made it look like it ended (`SortCoursePointerArrayBySectionName()`
  was inside the same disabled region as the function above it, not
  separate) — the first cut left it calling now-removed symbols;
  `WITH_WERROR` build caught it immediately, inspection hadn't.
  Takeaway recorded in the backlog: always locate the actual matching
  `#endif` first. Several sites are legitimate compile-time selectors
  (`#if 0`/`#elif 1`/`#else` in `ScoreKeeperNormal.cpp`, `#if 0`/`#else`
  in `RandomSample.cpp`) or explained kept-for-reference code, not
  dead code — left untouched. ~13 sites remain for a follow-up pass.

* **Backlog item 15, second batch** (`f1d6e6c4ac`, 2026-09-04). Found a
  new shape of `#if 0` block: `RageUtil_AutoPtr.h` had three marked
  `#if 0 // broken VC6` — not dead code, but working `HiddenPtr<T>`
  functionality (cross-type converting ctor/assignment + the friend
  declaration it needs) disabled for a VC6 template bug. Toolchain
  floor is MSVC v143 (ADR 0001), nothing left to work around —
  **enabled** rather than deleted; zero risk since templates only
  compile if instantiated, and nothing instantiates the cross-type
  path today. Also removed two explicit debug-scaffolding blocks in
  `ScreenNameEntry.cpp` and a superseded generic template in
  `StdString.h`. Left `Player.cpp` and `ScreenEdit.cpp` alone — both
  read as plausibly-superseded-but-not-explicitly-disowned, and are
  gameplay/editor state-machine code fragile enough that a mechanical
  sweep shouldn't guess. Backlog now documents three block shapes to
  distinguish before touching any `#if 0`: plain dead code, an active
  `#if 0/#else` selector (never remove), and a toolchain-EOL block
  (enable, don't delete).

* **Backlog item 2 — first two MSVC warning categories promoted to
  `-Werror`** (2026-09-04). `C4189` (unused local) and `C4702`
  (unreachable code) removed from the `/wd` suppression list; all 15
  hit sites fixed first. Most were plain dead locals, but a recurring
  real pattern showed up multiple times: `FOREACH_X(v) return ...;` —
  a macro-generated `for` loop whose body always returns on its first
  iteration — makes MSVC prove the loop's back-edge unreachable and
  warn C4702 *on the loop line itself*, not on any code after it.
  Fixed by calling the underlying `GetNextX()` once and branching
  directly, preserving behavior exactly (including, in
  `ScreenGameplay::SaveReplay()`, a nested case where the *outer* loop
  needed to keep trying subsequent players when the *inner* one found
  nothing — a naive flatten would have silently changed that).
  `RageBitmapTexture.cpp` had an entire block that computed "better"
  texture dimensions and then did nothing with them (no log, no
  resize) — deleted outright as dead weight, not just the two flagged
  locals. Debug and Release surfaced *different* warning sets (Debug's
  weaker optimizer didn't fold away 3 sites Release did) — checked
  both before declaring the category clean. `C4100`/`C4244`/`C4267`
  remain suppressed; `C4100` is next (mechanical), `C4244`/`C4267` are
  the real ~4.4k-hit debt needing case-by-case review.

* **Backlog item 2 — C4100 measured + first two batches**
  (`713e58a0f6`, 2026-09-05). `baseline.md`'s "1362" figure for C4100
  was raw MSBuild lines, which double-counts a header's warning once
  per including TU; the real count is **314 unique sites**, spread
  across ~150 files with no concentration (max 11 in one file) — no
  "fix one file, mostly done" shortcut here, unlike C4189/C4702.
  Fixed the 12 highest-concentration files (83 sites) using this
  codebase's existing convention (comment out the unused name,
  `Type /* name */`) rather than `[[maybe_unused]]`, with one
  exception: `RageFileManager_ReadAhead.cpp::CacheHintStreaming()`'s
  parameter is genuinely used, just only inside
  `#if defined(HAVE_POSIX_FADVISE)` (unset on Windows) —
  `[[maybe_unused]]` there instead, since commenting the name would
  break the POSIX branch's compile. Two sites needed the actual body
  read, not just the pattern assumed: `OptionRowHandler.cpp` has four
  near-identical `ImportOption(OptionRow*, vpns, vbSelectedOut)`
  overrides in the same file and only some of them use `vpns`/
  `vbSelectedOut` — blindly commenting all four the same way would
  have broken the ones that do; `ScreenOptionsExportPackage.cpp`'s
  `sDirToExport` grep-matched a "use" that was actually inside a
  `/* XXX: totally doesn't work. -aj */` block comment, not live code.
  **Process note:** left `/wd4100` removed from `src/CMakeLists.txt`
  after the measurement build and almost committed that — caught it
  before committing by diffing against the last commit (should have
  come back to zero, since nothing was meant to change there yet).
  `/wd4100` stays suppressed; only 83 of 314 are done, promoting now
  would break `WITH_WERROR=ON` CI on the other 231.

* **Backlog item 2 — C4100 third batch, 159 of 314** (`9844ab3c4b`,
  2026-09-05). The 19 files sitting at exactly 4 sites each (76 more
  sites). Same mechanical shape, two more real exceptions found by
  reading each body instead of assuming: `RageFileManager.cpp`'s
  `GetDirOfExecutable(argv0)` is genuinely read on non-Windows
  (`#else` branches) — `[[maybe_unused]]`, not a commented-out name,
  so the identifier stays valid there; `RageSound.cpp`'s
  `GetSourceFrameFromHardwareFrame(bApproximate)` is mid-deprecation
  per an existing TODO ("part of a gradual procedure to remove
  bApproximate from the code base") — commenting it out here matches
  that plan rather than jumping ahead of it. 155 sites remain, all in
  files with 1-3 hits each (the long tail, no more "clear one file,
  get several" efficiency from here).

* **Backlog item 2 — C4100 fourth batch, 198 of 314** (`26b7de158b`,
  2026-09-05). The 13 files sitting at exactly 3 sites each (39 more
  sites). Same convention throughout. Two spots needed the full body
  read rather than trusting the grep-matched signature:
  `NoteDataUtil.cpp`'s `LoadTransformedLights`/`CopyLeftToRight`/
  `CopyRightToLeft` and `RageDisplay_OGL.cpp`'s
  `GetTextureDiagnostics` all had their flagged params referenced only
  inside `/* ... */` block comments — genuinely dead, not live usage.
  `GameState.cpp::GetHumanPlayers` was the inverse surprise: it's `p`
  that's unused, not `L` (`L` is passed on to
  `LuaHelpers::CreateTableFromArray(vHP, L)`). Verified Release build
  clean under `WITH_WERROR=ON` (zero C4100 in the 13 files, zero
  warnings overall in the `sm_tests` rebuild), `--SelfTest` exit 0,
  `sm_tests` 94 assertions / 19 cases pass. `src/CMakeLists.txt`
  confirmed zero-diff before commit (the near-miss from the first
  batch made this a standing check now). 116 sites remain, all in
  files with 1-2 hits each.

* **Backlog item 2 — C4100 fifth batch, 252 of 314** (`3636b80690`,
  2026-09-05). The 27 files sitting at exactly 2 sites each (54 more
  sites), spanning screens, arch input/loading-window backends, and
  Rage subsystems. No new exception shapes this round -- every site
  was a genuinely unused parameter, confirmed by reading each
  function's full body rather than trusting the flagged signature
  alone. Verified Release build clean under `WITH_WERROR=ON` (zero
  C4100 in the 27 files, zero warnings overall in the `sm_tests`
  rebuild), `--SelfTest` exit 0, `sm_tests` 94 assertions / 19 cases
  pass. `src/CMakeLists.txt` zero-diff before commit. 62 sites remain,
  every one in a different file (the true single-hit tail).

* **Backlog item 2 — C4100 closed, 314 of 314; promoted to -Werror**
  (`c1d77d662a`, 2026-09-05). The last 62 single-site files. Two spots
  needed extra care to avoid editing the wrong overload/declaration:
  `ScreenTextEntry.cpp` has `SetTextEntrySettings` and `TextEntry`
  with near-identical parameter lists (only the former's `bPassword`
  goes unused -- confirmed by checking each function's own body, not
  just the flagged column); `InputHandler_Win32_ddrio.cpp`'s
  `crt_thread_create` has a forward declaration and a definition with
  the same signature, only the definition (with a body) triggers the
  warning. With all 314 originally-measured sites fixed, `/wd4100` is
  removed from `src/CMakeLists.txt` permanently -- promoted to
  `-Werror` alongside `C4189`/`C4702`.

  Verification went beyond the per-batch spot check this time: a full
  Release rebuild with `/wd4100` removed showed **zero** `C4100`
  across the *entire* `src/` tree, not just the 62 touched files --
  confirming no site was missed anywhere in the codebase, including
  files never touched by any of the five C4100 commits. `sm_tests`
  was then rebuilt under the existing `WITH_WERROR=ON` `build-tests`
  config (a second full recompile, since the CMakeLists.txt flag
  change touches every translation unit) -- zero warnings under
  `/WX`, 94 assertions / 19 cases pass. `--SelfTest` exit 0.

  Backlog item 2 now has only `C4244`/`C4267` (numeric
  conversion/narrowing, ~4.4k raw hits) open -- real debt needing
  case-by-case truncation review, not a mechanical pass like C4100.
  Not started; flagged for a future session with a scoping
  conversation first, given the much larger surface area.

* **Backlog item 13 — dropped stale VC6/VC2005 cruft from
  `arch_setup.h`** (`a26c13e00c`, 2026-09-05). `_CRT_SECURE_NO_DEPRECATE`
  and `_SCL_SECURE_NO_DEPRECATE` are VC2005-era macro names now
  superseded/no-op on the MSVC v143 floor (the modern
  `_CRT_SECURE_NO_WARNINGS` is already set at the CMake level; the SCL
  checked-iterator feature these suppressed was removed in VS2017).
  Also removed the ~30-line comment block documenting warnings
  disabled circa VC6/VC2005/VC2008 -- stale relative to the current
  `/W4` + `WITH_WERROR` setup (its C4100/C4702 mentions are now
  `-Werror`, see item 2 above). Kept `_CRT_NONSTDC_NO_WARNINGS`
  (POSIX-name deprecation, e.g. `strdup` vs `_strdup` -- still
  functionally relevant). Verified: Release build clean, `--SelfTest`
  exit 0.

* **Backlog item 3 — stale cppcheck leak list re-triaged, all
  dismissed** (docs-only, 2026-09-05). `Docs/Devdocs/possible memory
  leaks.txt` dates to a 2009 cppcheck run against sm4svn and was never
  re-verified. cppcheck itself isn't installed on the maintainer box,
  so each of the 11 in-scope entries was checked by hand: read the
  actual ownership path at the flagged site rather than trusting the
  tool's report. Findings, grouped by why cppcheck got it wrong (or
  why it's since been fixed):
  - **False positives cppcheck can't model:** `ActorFrameTexture.h`'s
    `m_pRenderTarget` (ownership passed to `TEXTUREMAN`, released via
    `UnloadTexture` in the destructor -- comment says so explicitly);
    `AutoKeysounds.h`'s `m_pSharedSound` and
    `RageSoundReader_PitchChange.h`'s `m_pSpeedChange`/`m_pResample`
    (both self-documented as "owned by"/"freed by" the reader chain
    they're threaded into); `RageSoundReader_ChannelSplit.h`'s
    `m_pImpl` (explicit `m_iRefCount` + `RageSoundSplitterImpl::Release`);
    `GameSoundManager.cpp`'s `pSound` (freed in `~MusicPlaying`);
    `LifeMeterTime.h`'s `m_pStream` and `MusicWheelItem.h`'s
    `m_pTextSectionCount` (both freed in their own class's destructor);
    `OptionRow.h`'s `m_textTitle` (added as an `ActorFrame` child, freed
    by `m_Frame.DeleteAllChildren()` in `Clear()`); `RageFile.cpp`'s
    `pFile` (same Lua-script-managed lifetime -- `PushSelf`/Luna
    binding, explicit `:destroy()` -- used by every other Luna-wrapped
    class in this codebase, e.g. the `RageMath.cpp` bezier classes from
    the C4100 sweep).
  - **Already fixed since 2009:** `Font.cpp`'s `pPage` (a comment at the
    exact site says "Create this down here so it doesn't leak if the
    continue gets triggered" -- the fix predates this re-triage by
    years); `RageFileDriverDeflate.cpp`'s `mem` (now wrapped in
    `std::unique_ptr` on entry to `GunzipFile`, so every return path,
    including early error returns, is RAII-safe).
  - **No longer applicable:** `AdjustSync.h`'s `s_pTimingDataOriginal`
    was refactored into `std::vector<TimingData>
    s_vpTimingDataOriginal` -- value semantics, nothing to leak.
  - **Not a bug:** `RageThreads.cpp`'s `pLock` in `GetThreadSlotsLock()`
    is a deliberate Meyer's-singleton program-lifetime static, never
    meant to be freed before process exit.
  - **Gone:** `PitchDetectionTestUtil.cpp` and `crypto/CryptRSA.cpp` no
    longer exist in this tree.
  - **Out of scope, not evaluated:** `archutils/Unix/CrashHandlerChild.cpp`
    (`tty`) and `smpackage/ZipArchive/Linux/ZipPlatform.cpp`
    (`mktemp`/`mkstemp` style note) are non-Windows paths (`AGENTS.md`
    §3).
  Updated the leak-list file itself with a triage note at the top
  (kept the original 2009 list below it for reference, rather than
  deleting -- `Docs/Devdocs/` is still-consulted reference material per
  `conventions.md`/`index.md`, not pure historical cruft). No code
  changed, no rebuild needed.

* **Backlog items 1/17 — TimingData characterization tests**
  (`197ea46f02`, 2026-09-05). ADR 0006 phase 2 continues past `RageMath`
  (which turned out to already be done -- `f98d7a489e`, added by the
  maintainer directly outside an agent session; the backlog just hadn't
  been updated to reflect it). New `tests/test_TimingData.cpp`: the
  beat<->row<->time core, exercised through the `NoOffset` entry points
  (`GetBeatFromElapsedTimeNoOffset`, `GetElapsedTimeFromBeatNoOffset`)
  that don't touch `GAMESTATE`/`PREFSMAN` -- their offset-applying
  callers (`GetBeatFromElapsedTime` etc.) are one-line wrappers, so this
  still pins the real logic without needing a live engine, matching the
  playbook's "not for code that needs a live GAMESTATE" rule.

  Covers: `NoteRowToMeasureAndBeat` across a time-signature change,
  `Has*Changes`/`Has*` predicates, `GetActualBPM`'s min/max/clamp,
  `IsWarpAtRow`'s half-open `[beat, beat+length)` interval, and
  constant-BPM / BPM-change / stop-holds-the-beat cases for the beat<->
  time conversion. Read `NoteRowToMeasureAndBeat`'s implementation
  closely before writing expectations -- its per-segment loop computes
  `iBeatIndexOut` with the same `rows / rows-per-measure` formula as
  `iNumMeasuresThisSegment` (not rows-per-beat), which looks like it
  could misbehave on multi-segment lookups; picked test rows that
  aligned with segment/measure boundaries to sidestep needing to fully
  untangle that before pinning behaviour, then verified predictions
  against the actual `sm_tests` run rather than trusting the hand-trace.
  134 assertions / 28 cases pass (up from 94/19). `src/CMakeLists.txt`
  untouched; only `tests/CMakeLists.txt` gained the new source file.

* **Backlog items 1/17 — NoteData characterization tests**
  (`6d4e6b5aa0`, 2026-09-05). ADR 0006 phase 2 continues past
  `TimingData`. A significant chunk of NoteData's public surface turned
  out to be out of scope: `GetNumTapNotes`, `GetNumMines`,
  `GetNumHoldNotes`, `GetNumRolls`, `GetNumLifts`, `GetNumFakes`,
  `GetNumRowsWithTap`, `GetNumRowsWithTapOrHoldHead`,
  `GetNumRowsWithSimultaneousTaps`, and `GetNumRowsWithSimultaneousPresses`
  all call `GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow()`
  either directly or through the private `IsTap`/`IsMine`/`IsLift`/
  `IsFake` helpers -- a live-GAMESTATE dependency the playbook rules
  out. `GetNumTapNotesNoTiming()` is the GAMESTATE-free counterpart and
  is covered instead; `RowNeedsAtLeastSimultaneousPresses` itself is
  pure (only its caller `GetNumRowsWithSimultaneousPresses` adds the
  GAMESTATE check), so it's tested directly too.

  New `tests/test_NoteData.cpp` covers: `SetTapNote`/`GetTapNote`
  roundtrip (including that writing `TAP_EMPTY` over an existing note
  erases the map entry rather than just resetting its type, and that a
  negative row is silently ignored), `GetFirstRow`/`GetLastRow`
  (including a hold's tail extending `GetLastRow` past its head row),
  the track-scanning family (`GetTapFirst*`/`GetTapLast*`/
  `GetFirstTrackWith*`/`GetLastTrackWith*` -- confirmed `HoldHead`
  doesn't count for the plain `*WithTap` variants, only the
  `*WithTapOrHoldHead` ones), `AddHoldNote`'s overlap-merge (a second
  hold overlapping an existing one extends it and absorbs the second
  hold's own head row) and underlying-tap-destruction behavior,
  `IsHoldNoteAtRow`'s exactly-at-the-head-returns-false quirk (the
  header already flags this with an "XXX: rename this to
  IsHoldBodyAtRow" comment) vs. `IsHoldHeadOrBodyAtRow` which does
  count the head, forward/backward row traversal via
  `GetNextTapNoteRowForTrack`/`GetPrevTapNoteRowForTrack`,
  `ClearRangeForTrack` truncating a hold at a range boundary,
  `GetNumTapNotesNoTiming`, and `RowNeedsAtLeastSimultaneousPresses`
  counting held (not just tapped) tracks toward the threshold once the
  direct-note count falls short.

  One authoring mistake caught before building: an early draft of the
  `RowNeedsAtLeastSimultaneousPresses` test asserted both `CHECK_FALSE`
  and `CHECK` for the identical threshold-3 call at the same row (the
  intent -- "one tap alone isn't enough, but adding two held tracks
  makes it enough" -- needs the assertions split across before/after
  adding the holds, not both checked against the same end state).
  Caught by re-reading the function's counting logic by hand before
  compiling, not by a failed build.

  A hand-trace of `IsHoldHeadOrBodyAtRow`'s missing-default third
  parameter (`pHeadRow`, no default value in the header) was skipped
  and caught instead by the compiler (`C2660`) on first build --
  fixed by passing `nullptr` explicitly.

  193 assertions / 40 cases pass (up from 134/28). `src/CMakeLists.txt`
  untouched; only `tests/CMakeLists.txt` gained the new source file.

* **Backlog items 1/17 — NoteDataUtil characterization tests**
  (`46001925f5`, 2026-09-05). ADR 0006 phase 2 continues past
  `NoteData`. Unlike `NoteData`'s own counting API, most of
  `NoteDataUtil`'s transforms operate purely on `NoteData` without
  touching `GAMESTATE` -- `RemoveFakes` in particular takes a
  `TimingData const&` parameter instead of reaching for the global
  timing data, which is exactly what makes it testable here (its
  sibling `IsJudgableAtRow` was already pinned in the `TimingData`
  file).

  New `tests/test_NoteDataUtil.cpp` covers: `RemoveHoldNotes`
  converting only `TapNoteSubType_Hold` heads to plain taps (Rolls
  untouched), `ChangeRollsToHolds`/`ChangeHoldsToRolls` swapping only
  the matching sub-type, `RemoveJumps`/`RemoveHands`
  (`RemoveSimultaneousNotes`) -- hand-traced its per-row removal loop
  carefully since it has two non-obvious quirks: the *last* pressed
  track at a row survives a cutdown, not the first (the loop clears
  tracks in ascending order until enough are gone), and a held track
  (mid-hold-body, no map entry at that exact row) is never itself
  removed but does count toward `iTotalTracksPressed` via
  `GetTracksHeldAtRow` -- `RemoveMines`/`RemoveLifts`/
  `RemoveAllTapsOfType`/`RemoveAllTapsExceptForType` filtering by exact
  type, `RemoveFakes` removing both explicit `TapNoteType_Fake` notes
  and anything landing under a non-judgable timing region (built a
  `TimingData` with a `FakeSegment` to exercise the latter),
  `RemoveAllButOneTap`, `ShiftLeft`/`ShiftRight`'s wrap-around track
  rotation (traced `ShiftTracks`'s `iFrom = i - iShiftBy` wrap formula
  by hand to get the direction right), `InsertRows`/`DeleteRows`
  round-tripping via `CopyRange`/`ClearRange`, `RemoveAllTapsOfType`/
  `ExceptForType`, `GetMaxNonEmptyTrack`, and
  `GetNextEditorPosition`/`GetPrevEditorPosition` -- traced through
  four successive calls by hand, including the boundary case where
  landing exactly on a hold's tail row makes the function report no
  further position (the `iEndRow == iOriginalRow` guard skips
  re-reporting the row you're already standing on).

  Every hand-traced prediction across both this file and the earlier
  `NoteData` file held on the actual `sm_tests` run -- no corrections
  needed after building, only a missing-argument compile error
  (`IsHoldHeadOrBodyAtRow` needs `nullptr` for its third parameter,
  which has no default) caught by the compiler in the prior file.

  246 assertions / 52 cases pass (up from 193/40). `src/CMakeLists.txt`
  untouched; only `tests/CMakeLists.txt` gained the new source file.
  This closes out `NoteData`/`NoteDataUtil` in the ADR 0006 phase 2
  sequence -- only a `NotesLoader*` corpus remains.

* **Backlog items 1/17 — NotesLoader characterization tests; Tier 1
  now empty** (`69803e8ca1`, 2026-09-05). Last file of ADR 0006 phase
  2. The `NotesLoader*` family's public surface splits cleanly into
  two halves, and only one is unit-testable without standing up the
  engine:

  - **The parse primitives** — `MsdFile::ReadFromString` (a pure
    in-memory tokenizer, zero globals) and the `SMLoader` string→timing
    helpers (`RowToBeat`, `ParseBPMs`/`ParseStops`,
    `Process{BPMsAndStops,Delays,TimeSignatures,Tickcounts}`) which,
    *on valid input*, touch nothing global. These turn simfile text
    into beats / rows / `TimingData` segments, so they are exactly
    where the `AGENTS.md` §5 "must keep loading identically" invariant
    bites. `tests/test_NotesLoader.cpp` pins them, quirks and all:
    `MsdFile`'s missing-`;` recovery at a line-leading `#`, its `//`
    comment skip and `\:` escape handling; `GetMainAndSubTitlesFrom
    FullTitle`'s five separators, the tab-before-`" -"` precedence, and
    the way the separator's leading space is dropped but its `(`/`-`/
    `~`/`[` half stays glued to the subtitle; `RowToBeat`'s `r`/`R`
    suffix → ÷`rowsPerBeat`; `ProcessBPMsAndStops` seeding a row-0 BPM
    segment and folding a pre-beat-0 stop into `m_fBeat0OffsetInSeconds`
    rather than keeping it as a stop; `ProcessTimeSignatures`
    back-filling an implicit `(0,4,4)` when the first entry isn't at
    beat 0; `ProcessTickcounts` clamping to `ROWS_PER_BEAT` (48).

  - **The file-loading entry points** (`NotesLoader::LoadFromDir`, the
    per-format `*Loader::LoadFromDir`/`LoadFromSimfile`) — scoped out.
    `RageFile::Open` hard-asserts `FILEMAN != nullptr`, `RageLog`'s
    constructor opens files through it, and `RageFileManager`'s
    constructor calls `LUA->Get()` — so even a bare `LOG` needs
    `FILEMAN` needs `LUA`. That is `--SelfTest` smoke territory per the
    characterization-test playbook ("not for code that needs a live
    GAMESTATE / renderer / audio device"). The `SMLoader` helpers'
    *error* branches (`"a=b=c"`, zero BPM, zero-length stop, negative
    beat) all call `LOG->UserLog()` and are out for the same reason.

  A committed simfile corpus driven by `GENERATE(from_range(...))` —
  the original phase-2 wish — stays open, but its real blocker is a
  shared Catch2 bootstrap fixture that news up `LUA`/`FILEMAN`/`LOG`
  once per run; the corpus files are the easy part. The same fixture
  would unblock salvaging `src/tests/test_file_readers.cpp` and
  `test_audio_readers.cpp`. Flagged in `baseline.md` and backlog item
  17 as a phase-3/4 prerequisite.

  All 20 new cases / 65 assertions predicted from a read of
  `MsdFile.cpp` + `NotesLoaderSM.cpp` held on the first `sm_tests` run
  — no build or assertion failures. Suite total **311 assertions / 72
  cases** (up from 246/52). `src/CMakeLists.txt` untouched; only
  `tests/CMakeLists.txt` gained the source file, so `WITH_TESTS=OFF`
  is unaffected. **Tier 1 of the backlog is now empty** — the safety
  net (smoke + harness + pure-ish-core characterization) is complete.

* **Backlog item 12 — `clang-tidy-subsystem-pass` #3: singletons ×
  `readability-container-size-empty`** (`204095fa27`, 2026-09-05).
  `--fix` over the 25 `.cpp` in `src/CMakeData-singletons.cmake`, one
  check family, no behavior change: `.size()==0` / `.size()<1` /
  `==""` → `.empty()`; `.size()` / `.size()>0` / `.size()>=1` /
  `!=""` → `!.empty()`. 14 files touched, **55 → 0** for this check
  across the subsystem. `RString::empty()` ≡ `==""`; every hunk
  reviewed — all touched conditionals are empty-guards
  (announcer/lights/screen-stack/theme-metric paths), no macros,
  template context, or `auto`-type changes; all edits landed in
  `.cpp`, zero headers (a `--header-filter` scoped to the 25 singleton
  header stems kept transitive headers out).

  **Tooling note:** the scratchpad LLVM 23.1.0 from passes #1–2 is gone
  (session-local `scratchpad/tools/`). Used the **VS-bundled
  clang-tidy 19.1.5** (`…\BuildTools\VC\Tools\Llvm\x64\bin\`) +
  VS-bundled `ninja` instead; the existing `build-tidy/
  compile_commands.json` (Sep 3) still resolved fine. Mechanical
  checks are version-stable, but `baseline.md`'s repo-wide totals
  table should be re-measured with one pinned version before its
  absolute numbers are trusted.

  **Verified on Windows, Debug only** — there is currently no Release
  `build/` tree on the maintainer box (needs a cold configure); for a
  pure emptiness-check swap, `build-tests` (`WITH_TESTS=ON`) building
  clean + `sm_tests` 311/72 + `StepMania-R_debug --SelfTest` exit 0 is
  proportionate. A Release + `WITH_WERROR` build is the stricter §4
  gate if the maintainer wants it re-run.

* **Backlog item 12 — `clang-tidy-subsystem-pass` #4: singletons ×
  `modernize-use-override`** (`22296571d5`, 2026-09-05). `--fix` over
  the 3 singletons `.cpp` with hits: added `override` (and dropped one
  redundant `virtual`) on 8 members that already override a base
  virtual — `GameStateMessageHandler::HandleMessage`,
  `ThreadedMemoryCardWorker`'s dtor + `HandleRequest` /
  `RequestTimedOut` / `DoHeartbeat`, `LocalizedStringImplThemeMetric`'s
  `Load` / `Read` / `GetLocalized`. **8 → 0** for this check across the
  subsystem; all edits in `.cpp`, no headers. Pairs with pass #3 on
  the same subsystem (mirrors what `rage` got: container-size-empty +
  use-override). Remaining singletons hits deferred as separate passes
  because they need review not rubber-stamping:
  `modernize-use-nullptr` ×6 (all `NetworkManager.cpp` — check for
  variadic calls) and `bugprone-macro-parentheses` ×8 (`StatsManager`
  ×4, + `NoteSkinManager` / `ProfileManager` / `ScreenManager` /
  `UnlockManager` ×1). Same Debug verification as pass #3.

## 2026-09-06

* **Test harness — shared engine bootstrap fixture (ADR 0006 phase 3-4
  enabler).** Added `tests/EngineTestEnv.{h,cpp}`:
  `EngineTestEnv::Require()` idempotently news up `LUA` → `FILEMAN` →
  `LOG` once per `sm_tests` run (that order is load-bearing —
  `RageFileManager`'s ctor calls `LUA->Get()`, and `RageLog`'s ctor
  opens a `RageFile` that `ASSERT`s `FILEMAN != nullptr`), mounts the
  new committed `tests/data/` corpus at `/testdata`, turns off `LOG`
  disk output, and a `CATCH_REGISTER_LISTENER` tears it all down at
  `testRunEnded`. Tests that never call `Require()` are unaffected.
  Paths reach the fixture via a `file(GENERATE)`d
  `EngineTestEnvPaths.h` using raw string literals (Windows backslashes
  need no escaping). **Deliberately not provided:** `PREFSMAN`,
  `GAMESTATE`, `GAMEMAN`, `THEME`, `SONGMAN`, renderer, audio — a full
  `#NOTES` parse still needs `GAMEMAN->StringToStepsType` and stays out
  of scope; the corpus files are song-tags only.
  First consumers in `tests/test_NotesLoaderFull.cpp` (41 assertions /
  6 cases): the `SMLoader::ParseBPMs`/`ParseStops` log-and-skip error
  branches (malformed expression / zero BPM / zero-length stop — these
  were called out as out-of-scope in `test_NotesLoader.cpp` *only*
  because `LOG` was null), and `SMLoader`/`SSCLoader::LoadFromSimfile`
  over a real committed `.sm`/`.ssc` pinning title/subtitle/artist/
  offset + the applied `#BPMS`/`#STOPS` timing (SM defers via
  `ProcessBPMsAndStops`; SSC applies per-tag). Also folded the "missing
  file → logs + returns false" branch.
  `src/CMakeLists.txt` untouched — only `tests/CMakeLists.txt` gains
  the new sources + the generated-header wiring, so `WITH_TESTS=OFF` is
  unaffected. Verified on Windows, Debug: `cmake --build build-tests
  --target sm_tests` clean under `WITH_WERROR=ON`, then
  `Program/sm_tests.exe` → **352 assertions / 78 cases pass** (up from
  311/72), `ctest` 100%. A Release / `--SelfTest` gate is unnecessary
  for an additive `tests/`-only change (same reasoning as the
  clang-tidy passes). Predicted-vs-actual: all 41 assertions held on
  the first `sm_tests` run — one compile fix en route (`SAFE_DELETE`
  needs `#include "RageUtil.h"`, not pulled by `global.h`).
  Unblocks the rest of ADR 0006 phase 4 (grow the corpus +
  `GENERATE(from_range(...))`) and the `src/tests/test_file_readers` /
  `test_audio_readers` salvage (they need the same `FILEMAN`).
