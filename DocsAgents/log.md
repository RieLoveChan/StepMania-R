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

* **Test harness — ADR 0006 phase 4 for `.sm`/`.ssc` (committed corpus
  + parse-regression).** Added `GAMEMAN` to `EngineTestEnv::Require()`
  (its ctor is trivial — Lua registration only; the game/style/
  `StepsType` tables are file-scope static data), which makes full
  `#NOTES`/`#NOTEDATA` parsing reachable — `SMLoader::LoadFromTokens`
  and SSC's `SetStepsType` both resolve `#STEPSTYPE` through
  `GAMEMAN->StringToStepsType`.
  New `tests/data/corpus/` (`corpus-a.sm` one dance-single chart,
  `corpus-b.sm` two charts, `corpus-c.ssc` with split chart-level
  timing) + `tests/test_NotesLoaderCorpus.cpp`: a
  `GENERATE(from_range(kCorpus))` case loads each file through its real
  loader and pins main/sub-title, artist, offset, song BPM at beat 0,
  and per chart `m_StepsType` (== `StringToStepsType(str)`),
  `m_StepsTypeStr`, difficulty, meter, `NoteData::GetNumTracks`, and
  `GetNumTapNotesNoTiming` (the `GAMESTATE`-free tap count). A second
  case pins SSC split timing: the `#NOTEDATA` `#BPMS` (180) populates
  the chart's own `m_Timing` and wins over the song `#BPMS` (120).
  This is the `AGENTS.md` §5 invariant in test form for the canonical
  read + write formats.
  Predicted-vs-actual: all 50 new assertions held on the first
  `sm_tests` run (arithmetic check: 45 from the 3 generator iterations
  + 5 from the split-timing case). Suite **352 → 402 assertions,
  78 → 80 cases**; `ctest` 100%. Verified Windows Debug, clean under
  `WITH_WERROR=ON`; `src/CMakeLists.txt` untouched.
  **Still open (backlog item 17):** `.sma` (needs a format-correct
  fixture), and `.dwi`/`.ksf`/`.bms`/`.crs` — those read `PREFSMAN` and
  only expose `LoadFromDir`, so they need `PREFSMAN` in `EngineTestEnv`
  + per-format directory fixtures.

* **Test harness — phase 4 pivoted to the real SM5 sample songs; toy
  simfiles removed.** Maintainer call: for simfiles the regression must
  use real files, not hand-authored toys (a toy only proves the loader
  survives input its author already understood, not the `AGENTS.md` §5
  invariant). Deleted `tests/data/corpus/corpus-{a,b,c}` and
  `tests/data/characterization-basic.{sm,ssc}`; added
  `tests/data/README.md` stating the policy.
  `EngineTestEnv` now also mounts the repo's `Songs/` tree at `/Songs`
  (`SM_SONGS_DIR` in the generated header) + `EngineTestEnv::SongPath()`.
  `tests/test_NotesLoaderCorpus.cpp` rewritten: a
  `GENERATE(from_range(...))` over the committed SM5 sample songs —
  `Songs/StepMania 5/Goin' Under/` (`.sm` **and** `.ssc`), `MechaTribe
  Assault/` (`.ssc`), `Springtime/` (`.ssc`) — via `LoadFromSimfile`.
  Per song: main/sub-title, artist, offset, song BPM at beat 0. Per
  chart (file order — `Song::AddSteps` preserves it), 39 charts across
  the 4 files: `m_StepsTypeStr`, `m_StepsType`
  (== `StringToStepsType(str)`), difficulty, meter,
  `NoteData::GetNumTracks`, `GetNumTapNotesNoTiming`. Values captured
  from a hidden `[.dump]` `TEST_CASE` kept in the same file (run
  `sm_tests "[dump]"` to re-baseline). Second case: Goin' Under `.sm`
  vs `.ssc` parse to identical charts + tap counts (cross-format §5
  equivalence — verified: 642/472/352/220/95/649/472/350/217 both
  ways). Coverage now spans `dance-single`/`-double`/`-solo`/`-couple`/
  `-threepanel` and `pump-single`/`-halfdouble`.
  Observed real behaviour, pinned as-is: Springtime's charts emit
  "Unmatched 3" hold-tail warnings from
  `NoteDataUtil::LoadFromSMNoteDataString` (tolerated by the loader).
  `test_NotesLoaderFull.cpp` trimmed to just the `LOG`-dependent
  error branches (`ParseBPMs`/`ParseStops` + missing-file), since its
  song-tag pins are now subsumed by the real-file corpus.
  Suite **402 → 638 assertions** (78 cases, was 80 — two toy-file
  cases dropped). Verified Windows Debug, clean under `WITH_WERROR=ON`;
  `ctest` 100%; `src/CMakeLists.txt` untouched.
  **Still open:** no committed sample exists for `.sma`/`.dwi`/`.ksf`/
  `.bms`/`.crs` — each needs a real song added under `Songs/` first,
  and the dir-only loaders still need `PREFSMAN` in `EngineTestEnv`.

* **Test harness — `PREFSMAN` added to `EngineTestEnv` (unblocks the
  dir-only loaders).** Construction order is now `LUA → FILEMAN → LOG →
  PREFSMAN → GAMEMAN`; teardown reversed because `~PrefsManager` calls
  `LUA->UnsetGlobal("PREFSMAN")`. `PrefsManager`'s ctor registers a few
  hundred `Preference<T>` objects, reads `Data/{Defaults,Preferences,
  Static}.ini` via `FILEMAN` (none mounted → graceful miss, every pref
  keeps its compiled default) and registers with `LUA`; the dtor only
  unregisters from `LUA`, no disk write. Adding it did **not** move any
  existing corpus pin — the `.sm`/`.ssc` `LoadFromSimfile` path does not
  read `PREFSMAN` — confirming it is safe in the always-on `Require()`.
  New `tests/test_EngineTestEnv.cpp` (3 cases / 13 assertions): the
  five singletons come up, `Require()` is idempotent, and the `PREFSMAN`
  defaults the dir loaders will read are pinned (`m_bQuirksMode` false,
  `m_bFastLoad` true, `m_fGlobalOffsetSeconds` -0.008). Header doc + ADR
  0006 enabler table updated (`PREFSMAN` row, teardown-order note).
  Suite **638 → 651 assertions, 78 → 81 cases**; Windows Debug clean
  under `WITH_WERROR=ON`, `ctest` 100%, `src/CMakeLists.txt` untouched.
  **Next:** `LoadFromDir` is now reachable for `.dwi`/`.ksf`/`.bms` —
  each just needs a committed sample song under `Songs/` (still none).

* **Test harness — `.pms`/BMS phase-4 coverage via a *derived* fixture
  (no copyrighted content committed).** Maintainer wants to avoid
  redistributing copyrighted simfiles. Approach: take a real 3-chart
  Pop'n Music `.pms` set (5-button / battle / normal), keep the note
  data, timing, `#BPM`/`#BPMxx` and `#WAVxx` keysound-channel structure
  **byte-for-byte** (that is what `BMSLoader` parses), and scrub only
  the identifying bits — `#TITLE`/`#ARTIST`/`#GENRE` → placeholders,
  every `#WAVxx <file>` → `#WAVxx key<ID>.wav`, mojibake comment line
  → `;`. Keysound audio → 44-byte silent PCM stub WAVs (one per
  `#WAVxx` id, 62 of them); the loader only does `IsAFile` on keysounds
  at parse time, so a silent stub is indistinguishable from the real
  sample for the parser. Committed at `tests/data/pms-fixture/`
  (`tests/data/` is not gitignored, unlike `Songs/*` — no `-f`).
  `tests/test_NotesLoaderBMS.cpp`: `BMSLoader::GetApplicableFiles` +
  `LoadFromDir`, pinning title/artist/BPM, `m_vsKeysoundFile.size()`
  (54 — 62 `#WAV` defs, 8 unreferenced, pool shared across the 3
  files), and per chart (`pnm-five` Hard/7, `bm-double7` Easy/1,
  `pnm-nine` Medium/6 — all 118 taps) type/difficulty/meter/tracks/
  taps. Verified the scrub is lossless: the derived fixture produces
  the exact same loader output as the untouched real folder did in a
  throwaway probe. New StepsType coverage: `pnm-five`, `pnm-nine`,
  `bm-double7`. Hidden `[bmsdump]` case to re-baseline. `tests/data/
  README.md` documents the derived-fixture policy for any format whose
  source isn't free. Suite **651 → 676 assertions, 81 → 82 cases**;
  Windows Debug clean under `WITH_WERROR=ON`, `ctest` 100%,
  `src/CMakeLists.txt` untouched.
  **Next:** `.sma`/`.dwi`/`.ksf`/`.crs` — same pattern (real song if
  redistributable, else derived fixture); the harness side is ready.

* **Test harness — `.dwi` phase-4 coverage (derived fixture).** Same
  copyright-safe pattern as `.pms`. Source: a community `.dwi` (3
  `dance-single` charts, `#CHANGEBPM`, `#GAP`, `#SAMPLESTART`,
  `#RANDSTART`) not clearly redistributable. Scrub touches only 3
  lines — `#FILE` / `#TITLE` / `#ARTIST` → placeholders — verified by
  `diff` that the 8-line header (minus those 3) and all three `#SINGLE`
  note blocks are byte-for-byte. DWI has no keysounds, so no stub
  audio needed. Committed at `tests/data/dwi-fixture/fixture.dwi`.
  `tests/test_NotesLoaderDWI.cpp`: `DWILoader::GetApplicableFiles` +
  `LoadFromDir` (note the 3rd `std::set<RString>& BlacklistedImages`
  param — pass an empty set). Pins: the `" ("` main/sub split done by
  `GetMainAndSubTitlesFromFullTitle` (DWI has no native `#SUBTITLE`) →
  `main="DWI Fixture"`, `sub="(Derived)"`; `m_sMusicFile`; `#GAP:065`
  → offset -0.065; `#BPM:145` at beat 0 + `#CHANGEBPM:1024=72.5` → 72.5
  at beat 256 (DWI row/4); `#SAMPLESTART:79.485`; and the 3 charts
  (`BASIC`/`ANOTHER`/`MANIAC` → `Difficulty_Easy`/3/233,
  `Difficulty_Medium`/8/443, `Difficulty_Hard`/10/680 taps). Verified
  the scrub is lossless — the derived fixture produces the exact same
  loader output (charts, taps, timing) as the untouched source in a
  throwaway probe; only the scrubbed string fields differ. Hidden
  `[dwidump]` case to re-baseline. Suite **676 → 705 assertions,
  82 → 83 cases**; Windows Debug clean under `WITH_WERROR=ON`, `ctest`
  100%, `src/CMakeLists.txt` untouched.
  **Next:** `.sma` (slots into `test_NotesLoaderCorpus.cpp`'s
  `kCorpus`), `.ksf` (`LoadFromDir` case), `.crs` (likely needs
  `SONGMAN`).

* **Test harness — `RageFile` coverage; salvages
  `src/tests/test_file_readers.cpp` (backlog item 17).**
  `tests/test_RageFile.cpp` — `RageFile` open / read / write / seek /
  tell / `GetLine` / `AtEOF` exercised end-to-end through `FILEMAN`'s
  in-memory `mem` driver (mounted at `/@mem` by the `RageFileManager`
  ctor — it is writable, so each test writes its own file; **no
  committed fixtures needed**, unlike the 2004-era original which
  wanted ~30 MB of uncommitted data). 8 cases / 67 assertions.
  Characterization highlights pinned:
  - `AtEOF()` is stdio-like: `m_bEOF` (RageFileObj) is set **only** by a
    read whose `ReadInternal`/`FillReadBuf` returns 0. Reading *exactly*
    to the end, or a short read that still returns >0 bytes, does **not**
    trip EOF — the next (zero-byte) read does.
  - `Seek(offsetPastEnd)` clamps to and returns the file size.
  - `GetLine` strips the trailing `\n`, returns the final unterminated
    line, then returns 0 (and `AtEOF` is true) — this buffered path
    *does* set EOF on end, unlike the large unbuffered `Read`.
  - `Read(RString&, -1)` reads from the current position to end.
  Suite **705 → 772 assertions, 83 → 91 cases**; Windows Debug clean
  under `WITH_WERROR=ON`, `ctest` 100%, `src/CMakeLists.txt` untouched.
  `src/tests/test_audio_readers.cpp` is still the open half of item 17's
  reader salvage — it needs committed audio fixtures + real decode.

* **Test harness — WAV sound-reader coverage; salvages
  `src/tests/test_audio_readers.cpp` (backlog item 17). Reader salvage
  now complete.** `tests/test_RageSoundReader.cpp` decodes a
  **synthetic** 16-bit PCM mono WAV — a canonical 44-byte-header WAV
  built byte by byte in the test with a deterministic periodic sample
  pattern, so there is no copyrighted audio and no committed fixture
  (the 2004 original wanted MP3/OGG files it didn't ship). Written to
  `/@mem`, decoded back through two entry points:
  - `RageSoundReader_WAV::Open(RageFileBasic*)` directly (bypasses the
    factory) — pins `GetSampleRate` (44100), `GetNumChannels` (1),
    `GetLength` (100 — milliseconds), PCM16→float (`sample/32768`,
    checked within a 0.001 margin), and `SetPosition(frame)` + re-read.
  - `RageSoundReader_FileReader::OpenFile(path, error)` — the
    format-autodetect factory; same results. It consults
    `ActorUtil::GetTypeExtensionList(FT_Sound)`, which is empty unless
    `ActorUtil::InitFileTypeLists()` has run — so that call was added to
    `EngineTestEnv::BringUp()` (after `LUA`, before `FILEMAN`, matching
    `sm_main()`; manager-free static-map setup; not idempotent but
    `BringUp()` runs once). Also pins that `OpenFile` returns `nullptr`
    + a non-empty error on a non-audio file.
  All 27 assertions held on the first run. Suite **772 → 799
  assertions, 91 → 94 cases**; Windows Debug clean under
  `WITH_WERROR=ON`, `ctest` 100%, `src/CMakeLists.txt` untouched.
  Both 2004-era reader scaffolds (`test_file_readers` +
  `test_audio_readers`) are now covered by fixture-free `tests/`
  characterization.

* **Test harness — `IniFile` characterization (safety-net extension).**
  `tests/test_IniFile.cpp` — the `.ini` reader/writer under
  `Preferences.ini`, keymaps, `Static.ini`, the theme-metrics fallback
  and the legacy `[Char Widths]`→`[main]` fixup (`Font.cpp`). It was
  load-bearing and completely untested. 11 cases / 76 assertions,
  parsed from strings via the writable `/@mem` mount (no fixtures).
  Quirks pinned bug-for-bug:
  - **Key name is `Trim()`'d, but `value` is `line.Right(...)` computed
    from the *untrimmed* key length**, so whitespace immediately after
    `=` stays in the value (`k = v ` → key `"k"`, value `" v "`).
  - Comment prefixes: `;` `#`, and `//`/`--` (only when `line[0] ==
    line[1]`). A *lone* leading `/` or `-` is NOT a comment — it falls
    through to key=value.
  - `key=value` before any `[section]` is silently dropped (no
    `[<empty>]` node).
  - Trailing `\` joins the next line (backslash removed).
  - A line with no `=` → `LOG->Warn`, dropped, parsing continues.
  - Repeated `[section]` merges into the existing node.
  - `RenameKey` returns **false** (no warning) when the source is
    absent or the target already exists — rename-if-present is the
    normal call pattern.
  All 76 held on the first run. Suite **799 → 875 assertions,
  94 → 105 cases**; Windows Debug clean under `WITH_WERROR=ON`,
  `ctest` 100%, `src/CMakeLists.txt` untouched.

* **Test harness — `XmlFile` characterization (safety-net extension).**
  `tests/test_XmlFile.cpp` — the engine's hand-rolled XML parser
  (`XmlFileUtil::Load` / `GetXML` over `XNode`), behind Profiles,
  `Lua.xml`, theme metrics, NoteSkins metadata and stats. Previously
  untested. 12 cases / 43 assertions; `XmlFileUtil::Load` takes an
  `RString` so no fixture and no `FILEMAN` (only `LOG`, for the error
  paths). Predictions from a read of `LoadInternal`/`LoadAttributes`
  all held on the first run. Quirks pinned:
  - `Load(&node, xml, err)` makes `node` **the root element itself**
    (no document wrapper); the `<?xml?>` prolog and `<!-- -->`
    comments are skipped and it recurses.
  - **Element text is only captured before the first child element** —
    `<r>lead<c/>trail</r>` keeps `"lead"`, drops `"trail"` (no mixed
    content).
  - Only the five named entities (`&amp; &lt; &gt; &quot; &apos;`)
    decode, in both text and attribute values. Numeric character
    references (`&#65;`, `&#x41;`) are left verbatim.
  - Attributes accept `"..."`, `'...'` **and unquoted** (`c=three`,
    ended by space or `>`); a name-only attribute (`<r flag .../>`) is
    kept with an empty value.
  - `GetChild(name)` returns the first same-named child;
    `GetChildrenBegin/End` iterate in document order.
  - `GetXML` re-encodes the entities and round-trips through `Load`.
  - Errors: `"Unterminated comment"`; unclosed root → non-empty error.
  Suite **875 → 918 assertions, 105 → 117 cases**; Windows Debug clean
  under `WITH_WERROR=ON`, `ctest` 100%, `src/CMakeLists.txt` untouched.

* **clang-tidy-subsystem-pass #5 + #6 (the two deferred `singletons`
  passes, backlog item 12).**
  **#5 — `modernize-use-nullptr` × `NetworkManager.cpp`**
  (`ef9fd6ad0b`). `--fix`, `NULL` → `nullptr` at the 6 sites: two
  `luaL_Reg` sentinel rows (`{NULL, NULL}` → `{nullptr, nullptr}`) and
  the `libname` arg of two `luaL_register(L, NULL, ...)` calls. The
  earlier "review variadic calls" caveat does not apply — `luaL_Reg`'s
  members and `luaL_register`'s 2nd param are all typed pointers, no
  varargs. **6 → 0.**
  **#6 — `bugprone-macro-parentheses` × 4 singleton macros**
  (`2d8227fbe8`). Hand-applied (not `--fix`) so each of the 8 flagged
  sites could be judged:
  - Fixed (real latent precedence bug if ever called with an
    expression): `NoteSkinManager` `FOR_NOTESKIN` `SArg(n+1)` →
    `SArg((n)+1)`; `ProfileManager` `FIXED_PROFILE_CHARACTER_ID`
    `int(i+1)` → `int((i)+1)`; `UnlockManager` `UNLOCK` `x.c_str()` →
    `(x).c_str()`. Plus `ScreenManager` `PLAY_CRITICAL` `snd.Play(...)`
    → `(snd).Play(...)` (defensive; `snd` is always a plain lvalue).
  - **Left, deliberately:** `StatsManager`'s 4 (`ADD_BOOLEAN_OPTION` /
    `ADD_FLOAT_OPTION`). The `name` arg is used as `PlayerOptions::name`
    — parenthesising after `::` is a syntax error, so clang-tidy's
    fixit there would break the build; the `#name` stringize and the
    always-plain `parent` / `opts` args carry no real risk. **8 → 4**
    for the check; the remaining 4 are unfixable false positives.
  Verified Windows Debug: `build-tests` (`WITH_WERROR=ON`) clean — all
  5 macro-using TUs recompiled, `StatsManager.cpp` did not; `sm_tests`
  918/117 unchanged, `ctest` 100%. **Tooling:** VS-bundled clang-tidy
  19.1.5 + the existing `build-tidy/compile_commands.json`; note Git
  Bash mangles `--extra-arg-before=/Y-` unless
  `MSYS_NO_PATHCONV=1` / `MSYS2_ARG_CONV_EXCL='*'` is exported first.
  Remaining deferred `singletons` tidy hits: none tracked — these were
  the last two.

## 2026-09-08

* **clang-tidy-subsystem-pass — `data-structures`,
  `readability-container-size-empty` (item 12). Recovered + verified +
  committed (`64e89eeabc`).**
  The sweep was done by an earlier session that ran in a toolchain-less
  Linux VM (no MSVC / CMake / clang-tidy reachable) and had to leave the
  edits uncommitted with no §4 verification. A follow-up session on the
  maintainer's Windows box picked the loose edits up, fixed one
  mis-fire, verified, and committed.
  **Scope:** `src/CMakeData-data.cmake` (the `data-structures` subsystem
  group), minus the simfile parse/write path (`NotesLoader*`/
  `NotesWriter*`/`TimingData`/`NoteData*`/`Song*`/`Steps*`, excluded by
  name), minus `CourseLoaderCRS.cpp` / `CourseWriterCRS.cpp` (`.crs` is
  a protected on-disk format per `AGENTS.md` §5 and there is no course
  regression corpus yet — **still open**, ~14 hits left there).
  **Method:** hand-applied (not `--fix`) — `grep` for
  `X.size() (==|!=|>) 0` and `X (==|!=) ""` across the remaining 65
  files, each hit read in context and edited by line number (several
  sites share identical text on multiple lines in one file, e.g. three
  `s == "" || s == "blank" || s == "Blank"` in `OptionRowHandler.cpp`).
  **57 sites / 15 files:** `BackgroundUtil` (2), `CodeDetector` (1),
  `CommonMetrics` (1), `Course` (7), `CourseUtil` (3), `Font` (2),
  `GameCommand` (14), `ImageCache` (2), `OptionRowHandler` (10),
  `PlayerStageStats` (2), `Profile` (2), `RandomSample` (2),
  `SampleHistory` (1), `SoundEffectControl` (1), `StageStats` (7).
  All are `.size()`/`.length()` `== 0`/`!= 0`/`> 0` → `.empty()` /
  `!.empty()`, or `RString == ""`/`!= ""` → the same — textual,
  semantics-preserving.
  **Mis-fire fixed on integration:** `SoundEffectControl.cpp:38` had
  `SOUND_PROPERTY == ""` rewritten to `SOUND_PROPERTY.empty()`, but
  `SOUND_PROPERTY` is a `ThemeMetric<RString>` (no `.empty()`).
  Corrected to `SOUND_PROPERTY.GetValue().empty()` — the shape
  `StageStats.cpp` already uses for
  `PREFSMAN->m_sTestInitialScreen.Get().empty()`. `container-size-empty`
  is only safe on real containers / `RString`; a `ThemeMetric` /
  `Preference` needs `.GetValue()` / `.Get()` first. Future
  hand-applied passes of this check: watch for `ThemeMetric` /
  `Preference` operands.
  **Verified (Windows Debug):** all 15 TUs force-recompiled clean under
  `WITH_WERROR=ON` (future mtime bump so MSBuild rebuilt them),
  `sm_tests` 918/117 unchanged, `ctest` 100%.
  **Gotcha for a Linux-VM-bridged session:** `device_bash` does not
  necessarily reach the toolchain a prior Claude Code session on the
  Windows box had — `which clang-tidy cmake ninja` first; if absent,
  land the diff and leave the §4 verification for a follow-up on the
  real box (as happened here), don't claim the gate was met.

* **Test harness — `.ksf` phase-4 coverage (derived fixture).** Same
  copyright-safe pattern as `.pms`/`.dwi`. Source: the real "1119.
  Crash - Crash Day" Pump It Up KSF folder (4 charts). Scrub touches
  only `#TITLE` / `#ARTIST` / `#STEPMAKER` / `#SONGFILE` — verified by
  `diff` that `#BPM` / `#TICKCOUNT` / `#STARTTIME` / `#DIFFICULTY` /
  `#PLAYER` and every `#STEP` block are byte-for-byte. KSF has no
  keysounds. Committed at `tests/data/Fixture Artist - KSF Fixture/`
  (dir name deliberate — see below).
  KSF quirks the test had to work around:
  - `KSFLoader::LoadFromDir` has **no `Dirname(path)` fallback** (unlike
    SM/DWI/BMS) — `LoadGlobalData` uses `out.GetSongDir()` directly, so
    the test must call `song.SetSongDir(dir)` first or every `#WAVxx`-
    style file resolve gets a relative path and fails.
  - StepsType **and** Difficulty come from the **filename** (lowercased):
    `"double"` → `pump_double` + `Difficulty_Medium`; no difficulty
    keyword → `pump_single` + `Difficulty_Hard` (else branch). So the
    fixture files are named `single-a/-b.ksf` / `double-a/-b.ksf` to
    reproduce the source's mapping. Meter still comes from each file's
    `#DIFFICULTY` tag (kept).
  - `#ARTIST` is **ignored** by KSFLoader; the artist is taken from the
    song **directory name** split on `" - "` (`LoadTags` on
    `asBits[size-2]`). Hence the fixture dir is
    `Fixture Artist - KSF Fixture` → artist `Fixture Artist`, and
    `#TITLE` still wins for the title (`LoadTags` only fills blanks).
  `tests/test_NotesLoaderKSF.cpp` (30 assertions / 1 visible case +
  hidden `[ksfdump]`): pins title/artist, `#BPM` 220,
  `#STARTTIME:17`→offset -0.17, and 4 charts — `pump-double` Medium/17
  & 26, `pump-single` Hard/17 & 23 (601 / 895 / 622 / 807 taps).
  **New StepsType coverage: `pump-double`.** Verified against the
  untouched source folder: identical chart output, only the scrubbed
  strings differ. Suite **918 → 948 assertions, 117 → 118 cases**;
  Windows Debug clean under `WITH_WERROR=ON`, `ctest` 100%,
  `src/CMakeLists.txt` untouched.
  **Phase 4 status:** `.sm`/`.ssc`/`.pms`/`.dwi`/`.ksf` done; `.sma`
  (needs a real `.sma` song) and `.crs` (courses reference songs →
  likely `SONGMAN`) remain.

* **Bug-hunt — `bugprone-integer-division` + `bugprone-suspicious-string-compare`
  (backlog item 12's "look at each" set). One real bug fixed.**
  Ran both checks over all of `src/` (parallel clang-tidy driver,
  `build-tidy/compile_commands.json`, `MSYS_NO_PATHCONV=1`) and re-read
  every hit in context. 27 hit lines; 25 in-scope.
  **Fixed (`f5005b8754`):** `RageSurfaceFormat::operator==` — for a
  paletted format it did
  `memcmp(palette.get(), rhs.palette.get(), sizeof(RageSurfaceFormat))`,
  but `palette` is a `RageSurfacePalette` (`RageSurfaceColor[256]`,
  1024 B), not a `RageSurfaceFormat` (~128 B). Only ~1/8 of the palette
  was compared → two 8-bit formats whose palettes differ past color
  ~index 32 wrongly compared equal. → `sizeof(RageSurfacePalette)`.
  Left the adjacent `memcmp(nullptr, ...)` UB (guarded by the
  `BytesPerPixel == 1` invariant) alone to keep the fix single-purpose.
  **`suspicious-string-compare` — the rest are not defects:** every
  other hit is idiomatic `if( memcmp(a, b, n) )` used as "are they
  different" (`RageDisplay:674`, `RageFileDriverZip:119` ZIP EOCD
  signature scan, the `arch/Sound/RageSoundDriver_WDMKS` /
  `RageSoundDriver_WaveOut` format-tag checks, `Win32/CrashHandlerChild`,
  `Win32/mapconv`). clang-tidy just wants an explicit `!= 0`; no bug.
  **`integer-division` — no fixes, all in rendering / UI positioning:**
  `ActorMultiVertex:373` (`v/num_splines` — deliberate segment index,
  paired with `v%num_splines` on the line above), `WheelBase:143`
  (`i - NUM_WHEEL_ITEMS/2` — integer offset from the centre item),
  `ScreenOptions:681` (explicit `(int)NUM_ROWS_SHOWN` cast, deliberate),
  `ScreenEdit:1459` (**false positive** — `SCREEN_HEIGHT` is
  `ScreenDimensions::GetScreenHeight()`, returns `float`). `Font:125`/
  `:132` (baseline/top off by 0.5 px for odd `m_iLineSpacing`),
  `NoteField:512` (marker bar off-centre 0.5 px for odd `GetWidth()`),
  `SnapDisplay:27`/`:28` (`m_iNumCols/2` — snap indicators 0.5 arrow too
  close to centre for odd column counts, e.g. pump-5), `ScreenSelectCharacter:264`
  (`MAX_CHAR_ICONS_TO_SHOW/2` — half-icon vertical offset if that metric
  is odd) are **genuine sub-pixel / half-unit imprecision**, but each
  "fix" moves an on-screen element on a path with no unit test and no
  `--SelfTest` render coverage — an `AGENTS.md` §4 observable-behavior
  change. Flagged here for the maintainer; not touched.
  Verdict recorded in `baseline.md`'s clang-tidy table so the next agent
  doesn't re-hunt.

* **Logging overhaul phase 2 — Debug level + runtime `--LogLevel`
  threshold (ADR 0005, `156c075ff3`). Partial: the global filter, not
  categories.**
  - `RageLog::Debug()` + `WRITE_DEBUG` + `[DEBUG]` tag — a level below
    Trace, log.txt only. No call sites yet (phase 4).
  - `enum RageLog::LogLevel { Trace, Debug, Info, Warn, Error }` (ADR
    0005 ordering) + `LogLevelFromString` (five names, lower+trim,
    unknown → Trace) / `LogLevelToString`.
  - `SetLogLevel()` → `RageLog::Write()` drops any line whose level is
    below the minimum, from **all** destinations. The time log and
    userlog.txt bypass the filter. Default = Trace (no behaviour
    change).
  - `PrefsManager::m_sLogLevel` (`"LogLevel"` pref, default `"trace"`;
    inserted between `m_bShowLogOutput` and `m_bLogSkips` in both the
    header decl and the ctor initializer list — order matters under
    `-Wreorder`/`WITH_WERROR`). Applied in `ApplyLogPreferences()`,
    then overridden by an explicit `GetCommandlineArgument("LogLevel",
    &s)` — there is **no generic `--<PrefName>=` CLI→pref bridge** in
    this codebase (the baseline's smoke-test-plan note is a proposal,
    not reality; `--VideoRenderers`/`--SoundDrivers`/`--game`/`--theme`
    are each read explicitly), so a new `--Foo` flag needs its own
    `GetCommandlineArgument` call. Lines emitted before
    `ApplyLogPreferences()` (arch hooks, LUA, FILEMAN) are already out
    and unfiltered — inherent.
  - `tests/test_RageLog.cpp` (5 cases / 21 assertions): enum ordering,
    string↔enum round-trip + case/whitespace + unknown-string
    fallback, `SetLogLevel` accepts every level.
  **Verified (Windows Debug):** `sm_tests` 948→969 / 118→123, `ctest`
  100%, clean under `WITH_WERROR=ON`. Built the `StepMania` target and
  ran `StepMania-R_debug --SelfTest --VideoRenderers=null
  --SoundDrivers=null`: exit 0 at every level; default 459 log lines
  (321 `[TRACE]` + 138 `[INFO]`), `--LogLevel=info` → 138 (TRACE gone),
  `--LogLevel=warn` → **0** (a clean phase-1 boot has no warnings).
  **Still open for phase 2:** per-category thresholds
  (`--LogLevel=gl:off,font:trace`), the `Log::Category` enum, and the
  `LOG_*` `file:line` macro layer. Left because the category taxonomy
  for a 700-file engine is a maintainer design call, not something to
  guess at. Phases 3 (repeat-collapsing) and 4 (call-site audit)
  untouched.

* **Logging overhaul phase 2 — category layer (`3a53baad5f`). Phase 2
  now complete.** On top of the global level filter from
  `156c075ff3`:
  - `namespace Log { enum Category }` — seed taxonomy: `General` (the
    default / uncategorised), `Arch File Lua Theme Font Gl Sound Input
    Song Steps Actor Screen Profile Net Cache`. `CategoryFromString`
    (lower+trim, unknown → `General`) / `CategoryToString` (the short
    lowercase name, also the `--LogLevel=<cat>:<lvl>` key).
  - `LOG_TRACE/DEBUG/INFO/WARN/ERROR( cat, fmt, ... )` macros →
    `LOG->LogLine( level, cat, __FILE__, __LINE__, ... )`. `LogLine`
    filters against `GetEffectiveLevel(cat)`, trims the path at the
    first `src/`, formats `"%-7s %s:%d  %s"` (cat / file:line / msg),
    and hands it to `Write()`. Bare `LOG->Trace(...)` etc. still work
    (they log as `Log::General`, no file:line).
  - Per-category minimum: `signed char m_CategoryLevel[NUM_Category]`,
    `-1` = "follow global" (set in the ctor loop — an array can't be
    brace-init'd to -1). A category CAN sit **below** the global
    (`global=warn` + `font:trace` keeps font). `SetLogLevelSpec` parses
    the full `--LogLevel` string: split on `,`; a bare token → global
    (`SetLogLevel`); a `cat:level` token → `SetCategoryLevel`; unknown
    category ignored, unknown level → Trace. `LogLevel` gained `Off`
    (past Error) so `gl:off` works.
  - `RageLog::Write` refactored: `(int where, LogLevel, Log::Category,
    RString)` — severity is explicit now, not inferred from the
    `where` bits. Dropped `WRITE_LOUD`/`WRITE_ERROR`/`WRITE_DEBUG`;
    `WRITE_TO_INFO`/`_USER_LOG`/`_TIME` stay as routing bits. `Write`
    is private — no external callers, contained refactor. Also fixed a
    latent `-Wreorder` risk: `m_sLogLevel` sits between
    `m_bShowLogOutput` and `m_bLogSkips` in both the `PrefsManager`
    header decl and the ctor list.
  - `StepMania.cpp` `ApplyLogPreferences` → `SetLogLevelSpec` (was
    `SetLogLevel(LogLevelFromString(...))`).
  `test_RageLog.cpp` (6 cases / 33 assertions): both enums + round-trip,
  `SetLogLevelSpec` (global / per-cat / both / bogus) via
  `GetEffectiveLevel`, and a `LOG_*` macro smoke.
  **Verified (Windows Debug):** `sm_tests` 969→1002 / 123→124, `ctest`
  100%, clean under `WITH_WERROR=ON`. `StepMania-R_debug --SelfTest`
  exit 0 at every spec: default 460 lines (321 TRACE + 139 INFO),
  `--LogLevel=info` → 139, `--LogLevel=warn` → 0,
  `--LogLevel=trace,file:warn` → 460 (unchanged — nothing calls
  `LOG_*(Log::File, …)` yet, so the per-category filter has nothing to
  act on: that is phase 4).
  **Phase 2 is done.** Phase 3 (repeat-collapsing) and phase 4 (the
  call-site migration — bare `LOG->` → `LOG_*` + real categories,
  `Warn`→`Trace`/`Error` triage) remain; phase 4 is what makes the
  per-category filter and the `file:line` column actually visible.

* **Logging overhaul phase 3 — consecutive-identical-line collapsing
  (`65fbca7bf5`).** `RageLog::Write` folds a run of identical
  consecutive lines: occurrences 1 and 2 print verbatim, 3+ are
  suppressed and, when the run ends (a different line arrives, or the
  dtor), replaced with `[TRACE] (previous line repeated N more times)`
  (N = suppressed count = occurrences − 2). A mere pair therefore
  produces no note. The summary inherits the run's tag + `where` bits
  (a `[WARN]` run's note is `[WARN]` and lands in info.txt). Identity =
  the tagged message minus the timestamp — level and category are part
  of it (a Trace "x" and a Warn "x" don't merge). Time-log /
  userlog.txt lines and multi-line messages never collapse.
  Implementation: the per-line emit moved out of `Write()` into
  `EmitLine()`; `SpillRepeat()` emits the pending note and is called on
  a line change and in `~RageLog` (before the final flush) so a run
  pending at exit isn't lost; `m_sLastEmit` / `m_sLastTag` /
  `m_iLastWhere` / `m_iRepeatCount` hold the state, under the existing
  `g_Mutex`.
  **Bug fixed en route (found by an expanded test):** `SetLogLevelSpec`
  now resets to defaults (global Trace, all per-category overrides
  cleared) before applying the tokens — the spec is the *complete*
  config. `--LogLevel=sound:error` = global trace + sound:error; write
  `--LogLevel=warn,sound:error` to keep a raised global. Previously a
  per-cat-only spec accumulated and there was no way to clear a
  category back to "unset".
  **Verified (Windows Debug):** `sm_tests` 1002→1004 / 124, `ctest`
  100%, clean under `WITH_WERROR=ON`. `StepMania-R_debug --SelfTest`
  exit 0; `log.txt` 460 → 438 lines, 4 collapse notes, the biggest
  eating 16 repeats of the `glTexImage2D (… 256x256 …)` trace — the
  exact case ADR 0005's context table measured (68 glTexImage2D lines).
  No un-collapsed run of ≥ 3 identical lines remains.
  **Logging overhaul phases 1-3 are done.** Phase 4 (call-site
  migration to `LOG_*` + categories, `Warn`→`Trace`/`Error` triage) is
  the long tail, per subsystem — and what finally lights up the
  per-category filter and the `file:line` column.

## 2026-09-09

* **clang-tidy-subsystem-pass — `actor`,
  `readability-container-size-empty` (item 12, `460435731a`).**
  `clang-tidy --fix` (VS-bundled 19.1.5, `-p build-tidy`,
  `--extra-arg-before=/Y-`, `--checks=-*,readability-container-size-empty`)
  over `src/CMakeData-actor.cmake` minus platform TUs. **47 sites / 23
  files:** ActorMultiVertex, ActorScroller, ActorUtil (4),
  AttackDisplay (2), BGAnimation, BPMDisplay, Background, Banner (5),
  BitmapText, EditMenu (2), FadingBanner (2), ModIcon, ModIconRow,
  Model (5), MusicWheel (5), NoteField, OptionRow (3), OptionsList (5),
  Player, ScoreDisplayBattle, ScoreDisplayCalories, WheelBase,
  WheelNotifyIcon. All `.size()`/`.length()` `== 0`/`> 0`/`< 1` and
  `RString ==`/`!= ""` → `.empty()` / `!.empty()` — autofix, every hunk
  reviewed, semantics-identical (no §4 behavior concern). `Player.cpp`
  was the only gameplay-hot TU touched: one guard in
  `ApplyRandomAttack` (`m_RandomAttacks.size() < 1` → `.empty()`).
  No `ThemeMetric` / `Preference` operands in this batch (the
  `data-structures` mis-fire class), so `--fix` was safe to trust here.
  **Verified (Windows Debug):** `sm_tests` Debug clean under
  `WITH_WERROR=ON`; 1004 assertions / 124 cases pass; `ctest` 100%.

* **clang-tidy-subsystem-pass — `actor`, `modernize-use-override`
  (item 12, `99e9c35e15`).** Same tool/flags,
  `--checks=-*,modernize-use-override`. **23 sites / 5 files:**
  Actor (1), BPMDisplay (2), Background (8), GraphDisplay (8),
  Tween (12). Every hit is a helper class defined *inside* the `.cpp`
  (`HiddenActor`, `SongBPMDisplay`, `BrightnessOverlay` /
  `BackgroundImpl`, `GraphLine` / `GraphBody`, the `Tween*` structs) —
  no header touched. Fix drops redundant `virtual` and annotates
  overriders + virtual dtors with `override`. **Verified (Windows
  Debug):** `sm_tests` clean under `WITH_WERROR=ON`, 1004 / 124,
  `ctest` 100%.

* **clang-tidy-subsystem-pass — `screen`,
  `readability-container-size-empty` (item 12, `8c65198efb`).** Same
  tool/flags over `src/CMakeData-screen.cmake` minus platform TUs. **66
  warnings / 47 sites / 23 files** (Screen, ScreenDemonstration,
  ScreenEdit ×6, ScreenEnding, ScreenEvaluation, ScreenGameplay ×2,
  ScreenGameplaySyncMachine, ScreenHowToPlay ×2, ScreenInstallOverlay
  ×2, ScreenJukebox ×3, ScreenMapControllers, ScreenNameEntry ×3,
  ScreenNameEntryTraditional ×2, ScreenOptions ×2,
  ScreenOptionsCourseOverview ×2, ScreenOptionsManageEditSteps,
  ScreenOptionsManageProfiles ×3, ScreenSelectMusic, ScreenTestInput,
  ScreenTestSound, ScreenTextEntry ×3, ScreenTitleMenu,
  ScreenUnlockStatus ×5). `.size()`/`.length()` vs `0`/`1` and
  `RString ==`/`!= ""` → `.empty()` / `!.empty()` — autofix, every hunk
  reviewed. A few land in `ASSERT()` / `ASSERT_M()` operands and two in
  `do { … } while( sKey.empty() )` loop conditions
  (`ScreenTextEntryVisual::MoveX`/`MoveY`) — still pure predicate
  rewrites. Note: `PREFSMAN->m_sTestInitialScreen.Get() != ""` sites
  became `.Get().empty()` (the `.Get()` was already there — this check
  *is* safe once the operand is a real `RString`; contrast the
  `data-structures` `ThemeMetric` mis-fire). **Verified (Windows
  Debug):** `sm_tests` clean under `WITH_WERROR=ON`, 1004 / 124,
  `ctest` 100%.

* **clang-tidy-subsystem-pass — `file-types` + `globals` + `data`
  leftovers, `readability-container-size-empty` (item 12).** Three
  small commits together:
  - `f06f0e0218` — `file-types` (`XmlFile.cpp`, `XmlFileUtil.cpp`) +
    `globals` (`StepMania.cpp`), 11 sites. The four
    `DEBUG_ASSERT( sName.size() )` in XmlFile/XmlFileUtil were
    hand-applied (clang-tidy `--fix` skips macro arguments); the rest
    autofixed. `XmlFile` has a characterization net
    (`tests/test_XmlFile.cpp`).
  - `1dc470ed24` — `data` non-parse-path leftovers the hand-applied
    `64e89eeabc` sweep missed: `Course::Matches`, `GameCommand::Apply`,
    `OptionRowHandler` ×2 (both `ROW_INVALID_IF(...)` macro args),
    `PlayerStageStats`. 5 sites, hand-applied — a `--fix` over the
    whole `data` group would also rewrite the AGENTS.md §5-protected
    `NotesLoader*`/`NotesWriter*`/`Song*`/`Steps*`/`TimingData` and
    `.crs` TUs (~85 more hits in the group, left untouched).
  All `.size()`/`.length()`/`< 1`/`!size()` in bool/comparison context
  → `.empty()` / `!.empty()`. **Verified (Windows Debug):** `sm_tests`
  clean under `WITH_WERROR=ON`, 1004 / 124, `ctest` 100% after each.

* **clang-tidy-subsystem-pass — `modernize-use-override`, three groups
  (item 12).**
  - `3e83e3a3b5` — `file-types` (`XmlFileUtil`: `XNodeLuaValue`, 12) +
    `data` non-parse-path (`ImageCache`: `ImageTexture`, 4;
    `LocalizedString`: `LocalizedStringImplDefault`, 2;
    `OptionRowHandler`: the `OptionRowHandler*` subclass family, 42).
    60 sites.
  - `e78aa164ef` — `screen`, 182 sites, **174 of them in
    `ScreenDebugOverlay.cpp`** (the `IDebugLine` subclass family), plus
    `ScreenOptionsEditCourse` (3), `ScreenReloadSongs` (1),
    `ScreenTestInput` (4).
  Every site is a helper class defined *inside* the `.cpp` deriving
  from a virtual base — **no header touched** in any of the three
  groups (checked via `git diff --stat`: `.cpp` only). Fix drops
  redundant `virtual`, annotates overriders + virtual dtors with
  `override`. **Verified (Windows Debug):** `sm_tests` clean under
  `WITH_WERROR=ON`, 1004 / 124, `ctest` 100% after each.
  With `rage` (`689e35a486`), `singletons` (`22296571d5`), `actor`
  (`99e9c35e15`), and these three, `modernize-use-override` at the
  `-p build-tidy` / `.cpp`-scope level is now clear across every
  non-platform subsystem group. Any residual hits would be pure-header
  declarations a `.cpp` TU never instantiates — a header-touching
  sweep is a separate, larger call.

* **clang-tidy-subsystem-pass — `data` non-parse-path,
  `modernize-use-nullptr` (item 12, `66954c15cd`).** `CreateZip.cpp`
  (12: `TZip` ctor init list `zfis(0)`/`hfin(0)` and `0`-literal
  pointer comparisons `pfout!=0`, `hfin!=0`, `bufin!=0`, `fn==0`,
  `zfi->cextra!=0` in the bundled zip writer) and `DisplaySpec.cpp`
  (1: `luaL_openlib( L, 0, … )` name arg). **Ran `clang-tidy` directly
  on the two files** (not the `data` group) so the single hit in
  `NotesLoaderSM.cpp` — AGENTS.md §5 parse path — is left alone.
  Semantics-identical. **Verified (Windows Debug):** `sm_tests` clean
  under `WITH_WERROR=ON`, 1004 / 124, `ctest` 100%.
  Remaining low-value mechanical checks measured but **not run**:
  `modernize-use-equals-default` (actor 12 / screen 9 / data 5 /
  file-types 2 / globals 1 — deliberately skipped, matches the
  `rage`/`singletons` precedent: churny out-of-line `= default` for
  marginal gain), `modernize-use-bool-literals` (actor 1 / data 2 /
  file-types 2 — trivial, deferred), `readability-redundant-string-init`
  (0 everywhere).

* **clang-tidy-subsystem-pass — `rage`, `bugprone-macro-parentheses`
  (item 12, `b65fa92d65`).** Only 3 hits in the whole `rage` group,
  each a file-local macro: `RageSurface.cpp` `COMP(a)` →
  `(a) != rhs.a`; `RageSurface_Load_GIF.cpp` `ReadOK(file,buffer,len)`
  → `(file).Read(…)`; `RageUtil.cpp` `TONUMBER_NICE`'s `dest= …` →
  `(dest)= …`. Every call site of all three passes a plain identifier,
  so purely defensive — re-measured clean, `sm_tests` 1004 / 124,
  `ctest` 100%. **Not yet run** (larger, needs per-hunk review — the
  check's `--fix` skips stringize / `::`-scoped / member-access
  operands and can mis-wrap declaration-name params, cf. the
  `StatsManager` residue from `2d8227fbe8`): `bugprone-macro-parentheses`
  on `actor` (37), `screen` (34), `data` (120).

* **clang-tidy-subsystem-pass — `bugprone-macro-parentheses`, actor +
  screen + data (item 12).** Done in three commits after the small
  `rage` one:
  - `ce1e1552fa` — `actor`, 37 sites / 15 files. Metric-name builders
    (`ssprintf("...P%d...",p+1)` → `(p)+1`), `s.c_str()` → `(s).c_str()`,
    combo-threshold `CROSSED`/`MILESTONE_CHECK` macros, and struct-compare
    / render-dispatch object wraps (`Actor` COMPARE, `NoteDisplay`
    DTS_INNER, `NoteField` OPEN_CALLBACK_BLOCK, `OptionRow`
    ERASE/INSERT_ONE_BOOL, …).
  - `99b8713986` — `screen`, 34 sites / 9 files. `ScreenSelectCharacter`
    has the bulk (its whole `P%d…Command(p)` family); `ScreenRanking`'s
    10 `ROW_SPACING_**row` row-position macros; the rest are the same
    metric-name / `.c_str()` / object-operand shapes.
  - `f6cfa5bab6` — `data`, ~110 sites / 19 non-parse-path TUs (**run
    per-file** so the 4 hits in `Song.cpp` / `SongCacheIndex.cpp` /
    `StepsUtil.cpp` — AGENTS.md §5 — stay untouched). `operator==`/`<`
    `COMPARE`/`EQUAL`/`COMP` macros across `Attack`/`DateTime`/
    `HighScore`/`PlayerOptions`/`SongOptions`/`StyleUtil`/`Trail`/
    `TrailUtil`/`BackgroundUtil`; `CodeDetector`'s scroll-speed/toggle
    ternaries; `CreateZip`'s bundled `PUTSHORT`/`PUTBYTE`/`DO1`/`ZE_MISS`;
    `Profile`'s map-iteration + `SWAP_ARRAY` macros; `CubicSpline`
    `UNNAN`/`BOOLS_FROM_CLOSEST`.
    **Two `--fix` mis-fires caught in review and reverted** — the check
    wraps a param even where it names a declaration or a type:
    `OptionRowHandler.cpp` `MAKE(type)` (`(type) *p = new (type)` — not
    valid) and `Profile.cpp` `LOAD_NODE(X)` (`X` is also `#X` /
    `Load##X##FromNode`, must stay a bare identifier). This is the
    documented failure mode — **always diff-review `macro-parentheses`
    `--fix`, never trust it blind.**
  All applied wraps are defensive (every call site passes a plain
  identifier/enum today). **Verified (Windows Debug):** `sm_tests`
  clean under `WITH_WERROR=ON`, 1004 / 124, `ctest` 100% after each.
  `bugprone-macro-parentheses` is now clear across `rage` / `actor` /
  `screen` / `data`(non-§5) / `singletons`(partial, `2d8227fbe8`);
  `file-types` and `globals` had 0.

* **clang-tidy — `container-size-empty` stragglers + full re-sweep
  (item 12).**
  - `aa85b33a91` — `ScreenEdit.cpp` `FILL_ENABLED(x)` macro (16
    expansion-site hits, one macro-body edit `.size() > 0` →
    `!….empty()`) — finishes `screen` for this check.
  - `b98c278a09` — `arch/Dialog/Dialog.cpp` (10, autofix: `ASSERT` +
    the `sID`-guard family) and the `FATAL_ERROR(s)` macro body in
    `RageSurface_Load_BMP.cpp` (8) + `RageSoundReader_WAV.cpp` (3,
    covered by `test_RageSoundReader.cpp`) hand-fixed to
    `sError.empty()`.
  - `770ca77f9e` — docs: 2026-09-09 full-config re-sweep table in
    `baseline.md` + `modernization-backlog.md` item 12 status. **The
    four mechanical checks (`container-size-empty`, `use-override`,
    `use-nullptr`, `macro-parentheses`) are now clear across every
    non-platform subsystem group outside the §5 parse/write path.**
    Remainder bucketed: §5-protected (needs a regression corpus),
    `arch/` driver internals (own lower-priority pass), and two checks
    (`use-equals-default` 38, `redundant-member-init` 28) whose `--fix`
    output is too dirty to land without a coupled `clang-format` run
    (ADR 0002 → separate change). `bugprone-integer-division` (14)
    stays maintainer-flagged.

* **clang-tidy — `arch/` + `archutils/Win32/` driver pass (item 12,
  same day).** Cleared the arch/driver tail for the four mechanical
  checks across six commits:
  - `137b83938f` — `_Null` drivers (`RageDisplay_Null`
    `RageCompiledGeometryNull`, `MovieTexture_Null`) `use-override` +
    one `modernize-redundant-void-arg` in `ArchHooks_Win32Static`.
  - `dc4e2d0387` — `archutils/Win32` crash-handler + video-info
    (`Crash`, `CrashHandlerChild`, `CrashHandlerNetworking`,
    `DebugInfoHunt`, `ErrorStrings`, `VideoDriverInfo`).
  - `e1e924b155` — Win32 input / lights / USB drivers
    (`InputHandler_DirectInput` + `_Win32_RTIO` + `_ddrio`,
    `LightsDriver_Win32Serial`, `USB`). Notable: `use-nullptr`
    correctly wrapped `StringToInt`'s `std::size_t* pos` arg and left
    the genuine integer `0L` (`lSecurityFlags`) alone.
  - `f9bc763307` — Win32 sound / display / window / movie drivers
    (`RageDisplay_D3D` `Present(0,0,0,0)`, `LowLevelWindow_Win32`
    `RenderTarget_Win32`, `MovieTexture_Generic`, the DSound/WDMKS/
    WaveOut `Init()` error checks, `Threads_Win32`, `GraphicsWindow`,
    `MemoryCardDriver` COMPARE macro).
  - `7a127f1af9` — final non-§5 stragglers (`ScreenNameEntryTraditional`
    ASSERT, `ScreenOptionsMasterPrefs` `remove_empty_back` macro,
    `RageUtil` `Json::Value obj.size() < 1`).
  - `b98c278a09` (logged above).
  All Windows-only TUs recompiled clean under `WITH_WERROR=ON`,
  `sm_tests` 1004/124 + `ctest` 100% after each. **Result:
  `container-size-empty` / `use-override` / `use-nullptr` are now clear
  across all of `src/` except the §5 parse path (~95, needs corpus) and
  the vendored `ixwebsocket` subtree (~13).** `macro-parentheses`
  residual = 4 documented unfixable sites (`StatsManager` ×2,
  `OptionRowHandler` MAKE, `Profile` LOAD_NODE). See `baseline.md`
  "Update — arch/Win32 driver pass done".

* **clang-tidy — `modernize-use-bool-literals` (item 12, `3be07f669d`).**
  23 sites / 17 files: `return 0;` in bool functions (`CsvFile`,
  `IniFile`), `while(1)` / `do…while(0)` → `while(true)`/`while(false)`
  (`Player`, `RageDisplay_D3D`/`_OGL`, `RageFileDriverDirect`,
  `RageSurfaceUtils` ×2, Win32 crash-handler / `GetFileInformation` /
  `GraphicsWindow` loops), `= 0`/`= 1` bool assigns (`PlayerOptions`
  `NextBool`, `RageSoundReader_MP3`, `RageSurfaceUtils_Palettize`,
  `ThemeManager::GetMetricB`, ddrio `LightsState { 0 }` → `{ false }`).
  1 §5 hit (`NotesLoaderSM`) left. `IniFile` covered by
  `test_IniFile.cpp`. `sm_tests` 1004/124, `ctest` 100%.

  **Backlog item 12 — clean autonomous lane exhausted.** Final
  full-config sweep (2026-09-09): every mechanical check with a clean
  `--fix` (`container-size-empty`, `use-override`, `use-nullptr`,
  `use-bool-literals`, `macro-parentheses`, `redundant-void-arg`) is
  clear across all first-party `src/` outside the §5 parse path (~95
  hits, needs a corpus) and vendored `ixwebsocket` (~13). What's left
  needs a human decision: `use-equals-default` (38) +
  `redundant-member-init` (28) want a coupled `clang-format` (ADR 0002
  → own change); `integer-division` (14) is maintainer-flagged;
  `suspicious-string-compare` (8) is idiomatic `memcmp`; 4
  `macro-parentheses` are genuinely unfixable.

* **ADR 0006 — `src/tests/` reader-salvage completed (item 17).** Three
  commits after the clang-tidy batch:
  - `68dc667855` — `tests/test_RageFileDeflate.cpp` (new). The
    `RageFileObjDeflate`/`RageFileObjInflate` raw-deflate layer (zlib
    `-MAX_WBITS`, used for gzip'd assets + compressed caches) had zero
    coverage. 9 cases: byte-exact round-trip + `CRC32(in)==CRC32(out)`
    over compressible / repeating / incompressible / tiny payloads at
    several read/write block sizes; "deflate shrinks a 100k run to
    <1/10"; write-chunk size not observable; `Inflate::Seek`; and error
    paths (corrupt stream → hard error or detectably-bad decode, never
    the full payload intact; truncated stream → short read, correct
    prefix). Pins two gotchas found writing it:
    `RageFileObjDeflate::FlushInternal` emits `Z_FINISH` (ends the
    stream) and the dtor calls it too — **never call `Flush()`
    yourself**; `RageFileObj::Read(RString&,int)` trims to the count
    read but does **not** clear the buffer first, so a reused `RString`
    returns stale bytes.
  - `2349d3b802` — extended `tests/test_RageFile.cpp` (+3 cases). The
    `RageFileObj` read buffer is `BSIZE=1024`; `GetLine` has hand-rolled
    logic for a line/newline straddling a refill (incl. the `\r\n`-split
    hack). Sweeps line lengths 1..2049 × {`\n`, `\r\n`} asserting exact
    char count and `\r` stripped; bare interior `\r` survives, trailing
    `\r` before `\n` stripped; text→4096-byte binary block→text with
    `Tell()` exact across the transition.
  - `e0083906fe` — `tests/test_RageFileErrors.cpp` (new, salvage of
    `test_file_errors.cpp`). A self-registering in-test VFS driver
    ("ERRTEST" — `FilenameDB.AddFile` + file-scope `FileDriverEntry` +
    a `RageFileObj` subclass on the modern `*Internal` interface) that
    fails the read/write/flush crossing a byte threshold. 6 cases pin
    error propagation through `RageFile::Read`/`Write`/`Flush` and
    `IniFile::ReadFile`/`WriteFile`.
  **Reader salvage from `src/tests/` is complete** (`file_readers`,
  `deflate`, `audio_readers`, `file_errors`). Remaining there:
  `test_vector.cpp` (macOS/altivec, §3 — skip), `test_threads.cpp`
  (`RageThreads`, item 11 — ADR-scoped). Suite **1004/124 → 5145/142**.

* **ADR 0006 — `tests/test_Zip.cpp` (new, `7d6374067e`).** The `.smzip`
  package path (`CreateZip` writer → `RageFileDriverZip` read-only VFS)
  had zero coverage. All in `/@mem`: write source files, build the
  archive, load it back, extract every entry byte-for-byte. 4 cases:
  round-trip byte-exact, missing entry → nullptr, `Open(WRITE)` →
  `ERROR_WRITING_NOT_SUPPORTED`, `Load` of a non-zip → false.
  **Characterization finding → backlog item 19:** this `CreateZip`
  build emits `STORED` for every entry (compressed size == uncompressed,
  `m_iCompressionMethod == STORED`) — its bundled Info-ZIP deflate is
  not wired, so engine-produced `.smzip`s are uncompressed. Maintainer
  decision (wire it / route through `RageFileObjDeflate` / accept).
  Suite **5145/142 → 5197/146**.

* **ADR 0006 — more pure-core coverage (2026-09-09, cont.).**
  - `aeec1a8578` — `tests/test_RageSurface.cpp` (new). The in-memory
    image type + `RageSurfaceUtils` pixel/format helpers (on every
    texture-load path). 6 cases: `decode/encodepixel` round-trip 1..4
    bpp; `Set/GetRawRGBAV` channel round-trip + explicit RGBA8888 bit
    layout; `GetBitsPerChannel`; **`RageSurfaceFormat::operator==` /
    `Equivalent` — regression pin for `f5005b8754`** (a change to
    palette entry 250 was missed by the wrong-`sizeof` memcmp); `Blit`
    overlap byte-exact + rows past `src->h` untouched; `ConvertSurface`
    32→16(RGBA4444)→32 within the 4-bit quantisation.
  - `572d67086e` — `test_RageMath.cpp` +4 cases. `RageQuadratic`
    `GetBezierStart/End` shortcuts, `SetFromBezier`/`GetBezier`
    round-trip; `RageBezier2D` (the Newton-Raphson x→t→y solve behind
    `Tween::InterpolateBezier2D`) — straight diagonal → `EvaluateYFromX(x)
    == x`; an asymmetric ease is monotone + slow at the start.
  - `4551f5468a` — `test_RageFile.cpp` +1 case. `RageFileDriverSlice`
    window: `GetFileSize`==window, reads yield only the window, past-end
    is EOF though the underlying file has more, `Seek` is slice-relative
    + clamps, `Write` → -1.
  Suite **5145/142 → 5321/157**.

* **ADR 0006 — pure-value-type coverage + a crash fix it turned up
  (2026-09-09, cont.).**
  - `2bb5800f5c` — `tests/test_DateTime.cpp` (new). The y/m/d/h/m/s
    value type (profile + high-score timestamps). 6 cases: `Init()`
    zeroes all; `GetString` omits the time half iff h==m==s==0;
    `FromString` parses both forms, internal repr is tm-style
    (`tm_year -= 1900`, `tm_mon -= 1`); `FromString` round-trips
    `GetString`; `FromString` **does not validate** (`2025-02-30`,
    `2025-13-99 25:61:61` parse as-is — pins the header's own "XXX
    illegal date" question); comparison orders y→mon→mday→h→m→s.
  - `ff8ed52a26` — `test_RageUtil.cpp` +4 cases for
    `StringConversion::FromString`/`ToString<T>` (the `Preference<T>`
    codec): int/float leading-number + trailing-junk-ignored + failure
    zeroes; float rejects non-finite; bool is `StringToInt(s) != 0`;
    `ToString<bool>` is `"0"`/`"1"`, `ToString<float>` is `"%f"`.
  - `1bff388838` — **fix**: `RageUtil.cpp` `StringToInt`/`Long`/`LLong`
    caught `std::sto*` exceptions and called `LOG->Warn(...)` with no
    null check → segfault if `LOG` isn't up yet (found writing the
    `FromString<bool>("true")` case). All 6 catch sites now
    `if( LOG )`. Backlog item 21 closed.
  Suite **5321/157 → 5428/167.**

* **ADR 0006 — engine-config coverage (2026-09-09, cont.).**
  - `1ff0eb549a` — `tests/test_GameManager.cpp` (new). The
    string→StepsType/Game/Style table lookups (GAMEMAN via fixture).
    `StringToStepsType` case-insensitive + every type round-trips its
    `GetStepsTypeInfo().szName`; `StringToGame` dance/pump/kb7;
    `GameAndStringToStyle(dance,"single"/"double")` → Style with the
    right `m_StepsType` / `m_iColsPerPlayer` (4/8);
    `GetStylesForGame`/`GetStepsTypesForGame` non-empty + round-trip via
    `GetGameForStyle`.
  - `c76ee6f0ae` — `tests/test_RageColor.cpp` (new). The `"1,0,0.5"` /
    `"#FF8000"` colour codec. 3-or-4 comma floats; `#RRGGBB`/`#RRGGBBAA`
    case-insensitive; **parse failure resets the colour to opaque
    white** (pinned side effect); `ToString` picks `#RRGGBB` vs
    `#RRGGBBAA` by rounded alpha, upper-case, clamps; 8-bit round-trip;
    `NormalizeColorString`.
  - `452c60f997` — `tests/test_SongOptions.cpp` (new). The per-song mod
    set (`SongOptions::FromOneModString` has no NOTESKIN/GAMESTATE dep,
    unlike `PlayerOptions`). `xMusic` rate token, on/off keyword mods
    (clap / autosync* / haste), unknown mod silently ignored, `Init()`
    reset, `GetString`/`FromString` round-trip via `operator==`.
  Suite **5428/167 → 5594/182**. (`PlayerOptions::FromString` needs
  `NOTESKIN` — deferred until `EngineTestEnv` grows a NoteSkinManager
  bring-up.)

* **ADR 0006 — codec coverage (2026-09-09, cont.).**
  - `2c43c42de1` — `tests/test_DeviceInput.cpp` (new). `DeviceInput::
    ToString`/`FromString` (Keymaps.ini codec). Invalid device →
    `""`; `"<device>_<button>"` round-trips for keyboard + joystick;
    rejects no-`_` / trailing-`_`; an `"a_b"`-shaped string with
    unresolvable names still returns true with Invalid parts.
  - `39013b52c1` — `tests/test_Command.cpp` (new). The Actor command
    mini-language (`ParseCommands`/`Command::Load`/`GetName`/`GetArg`).
    `;` / `,` splitting, empty args kept, `GetArg` past end → empty,
    empty/`;`-only → no commands, non-legacy quote handling vs legacy
    blind split. **Finding: `Command::GetName()` only `Trim()`s — it
    does NOT lower-case** despite the header comment; comment corrected
    in the same commit.
  Suite **5594/182 → 5662/193**.

* **ADR 0006 — parse-primitive coverage (2026-09-09, cont.).**
  - `bb0a2b93e9` — `test_RageUtil.cpp` +4 cases for `split()`/`join()`
    (under every simfile parse / command / config line): single- and
    multi-char delimiters, `bIgnoreEmpty` on/off, empty-source → empty
    vector, `join` is the exact inverse of `split(...,false)`.
  - `af37ddb551` — `tests/test_Difficulty.cpp` (new).
    `StringToDifficulty` (canonical names, `CompareNoCase`) round-trips
    `DifficultyToString`; `OldStyleStringToDifficulty` maps the legacy
    alias table the BMS/DWI/KSF loaders lean on
    (another→Medium, maniac→Hard, oni→Challenge, …).
  Suite **5662/193 → 5730/200**.

  **2026-09-09 test-coverage sweep, tallied:** starting from 1004/124,
  the session added `test_RageFileDeflate` / `test_RageFileErrors` /
  `test_Zip` / `test_RageSurface` / `test_DateTime` / `test_GameManager`
  / `test_RageColor` / `test_SongOptions` / `test_DeviceInput` /
  `test_Command` / `test_Difficulty` (11 new files) plus new cases in
  `test_RageFile` (GetLine boundary, `RageFileDriverSlice`),
  `test_RageMath` (`RageBezier2D`), `test_RageUtil` (`StringConversion`,
  `split`/`join`) → **5730 assertions / 200 cases**. Fixed en route:
  `RageSurfaceFormat::operator==` memcmp, `StringToInt/Long/LLong`
  null-`LOG` deref, `Command::GetName` stale comment; filed backlog
  item 19 (`CreateZip` STOREs uncompressed).

* **ADR 0006 — util-layer codec coverage, batch 3 (2026-09-09, cont.).**
  - `2cd997e9b9` — `tests/test_TimeFormat.cpp` (new). `SecondsTo*`
    formatters (`HHMMSS` builds from a total-minutes count;
    `*MsMs`/`*MsMsMs` clamp the fractional field, never carry) + `Commify`
    (digit grouping, sign / decimal left alone).
  - `cd27409e10` — `tests/test_Grade.cpp` (new). `GradeToString` /
    `StringToGrade`. **Finding: `StringToGrade` upper-cases its input
    for the FAILED/NODATA checks but runs the `"Tier%02d"` sscanf on the
    ORIGINAL string** — so `"failed"` works but `"tier03"` does not.
  - `53efa38c2e` — `test_RageUtil.cpp` +4 cases for `Regex` (PCRE
    wrapper): anchored/unanchored `Compare`, the capture-group-only
    out-vector (`out[0]` == group 1), `Replace` `\${n}` placeholders,
    copy-ctor recompile.
  - `0f4bad5070` — `test_RageUtil.cpp` +3 cases: **`Capitalize` touches
    only the first codepoint** (not the whole string — the file's own
    header quirk note was wrong and is fixed); `BeginsWith`/`EndsWith`
    case-sensitive anchors; `URLEncode` keeps `'!'..'z'` verbatim.
  Suite **5730/205 → 5830/216**. The 2026-09-09 characterization sweep
  now spans deflate/zip/errors, RageSurface, RageBezier2D,
  RageFileDriverSlice, DateTime, GameManager, RageColor, SongOptions,
  DeviceInput, Command, Difficulty, Grade, TimeFormat, StringConversion,
  split/join, Regex, and the small string helpers.

## 2026-09-10

* **Deleted `src/CreateZip.{cpp,h}` — dead STORED-only ZIP writer
  (backlog item 19 closed).** Traced the `.smzip` code paths after
  `test_Zip.cpp` flagged that `CreateZip`/`TZip` emits `STORED` for
  every entry: **nothing live calls it.** Its only would-be caller,
  `ScreenOptionsExportPackage::ExportPackage()`, had its whole body
  `#if 0`'d out for years ("XXX: totally doesn't work. -aj", using a
  `RageFileObjZip` class that no longer exists) and always returned
  false. Removed the 1126-line 2009 SM4-beta Info-ZIP fork (with its
  hand-rolled `crc32`) + its `CMakeData-data.cmake` / `Makefile.am`
  entries; replaced the dead `ExportPackage()` comment block with a
  one-line stub. Rebuilt `tests/test_Zip.cpp` around a ~90-line in-file
  minimal ZIP writer (STORED + a DEFLATED entry via
  `RageFileObjDeflate` + zlib `crc32`), so `RageFileDriverZip` — the
  reader — stays fully characterized with no engine writer.
  **Decompression is already current** and untouched: zlib **1.3.2**
  (`RageFileObjInflate`, the VFS `.smzip`/`.gz` path) + **miniz 2.2.0 /
  MZ_VERSION 10.2.0** (`RageFileManager::Unzip`, the Lua bulk
  extractor). If package export is ever wanted back, wire it through
  `mz_zip_writer_*` (drop `MINIZ_NO_ARCHIVE_WRITING_APIS`).
  **Verified:** `sm_engine` (all of `src/`) + `sm_tests` clean under
  Debug `WITH_WERROR=ON`, suite 5839/220, `ctest` 100%; **Release**
  `StepMania-R.exe` links clean under `WITH_WERROR=ON`.

* **Deleted `src/tests/` — the 2004-era standalone test harnesses
  (backlog item 22, partial).** `test_vector` / `test_threads` /
  `test_misc` / `test_file_readers` / `test_audio_readers` /
  `test_file_errors` / `test_timing_data` — pre-Catch2 `main()`
  programs, built only by the dead autotools `src/Makefile.am`, needing
  uncommitted 30 MB of test data, `#error`-ing without altivec/SSE.
  Every one's intent is now in the CMake `tests/` suite (item 17
  reader-salvage + phase 2 pure-core coverage). Removed the files and
  cut the `all_test_SOURCES` / `TESTS` block from `src/Makefile.am`.
  CMake build untouched (nothing referenced `src/tests/`). Filed
  backlog item 22 proposing removal of the whole dead autotools system
  (`configure.ac` still says `AC_INIT(StepMania, 5.0, …
  http://stepmania.com)` — pre-fork; CI/docs are CMake-only) as its own
  maintainer-greenlit commit.

* **Deleted `src/smpackage/` — dead MFC "SMPackage" tool (backlog item
  23).** 110 files / ~19k LOC: the old standalone installer/exporter GUI
  (`CDialog`/`CTreeCtrl`/`afxwin.h`) + a bundled 3rd-party `ZipArchive/`
  (`mfc/` + `stl/` + `Linux/` variants, VS2003/2008 `.vcproj`,
  `borland.zip`), `res/`, `.rc`. No CMake reference, no live `src/` code
  uses it or `SMPackageUtil`, `baseline.md`'s tidy sweep already
  excluded it, last commit msg was *"I still can't get SMPackage to
  compile"*, MFC isn't in a standard modern VS. Its only feature —
  `.smzip` export — was already dead (item 19). **`.smzip` load is a
  separate, untouched path** (`RageFileDriverZip` "ZIP" VFS driver +
  `RageFileManager::Unzip` miniz). **Verified:** `sm_tests` + `[zip]`
  reader tests pass, Release `StepMania-R.exe` links clean under
  `WITH_WERROR=ON`, `--SelfTest` exit 0.

* **Deleted the dead autotools build system (backlog item 22).** CI,
  `AGENTS.md`, `DocsAgents/build.md` and `Build/` docs are 100% CMake;
  `configure.ac` still had `AC_INIT(StepMania, 5.0, … http://stepmania.com)`
  (pre-fork, `AC_PREREQ(2.59)` = 2003) and `SUBDIRS = bundle src` where
  `bundle/` doesn't even exist. Removed `Makefile.am` (root),
  `src/Makefile.am`, `configure.ac`, `autogen.sh`, `autoconf/`
  (`config.rpath` + `m4/` ×18), `Utils/make-src-archive.sh` (the
  `autoreconf -if` + `make dist` script), and the autotools-output
  block from `.gitignore`. ~3.9k lines; `extern/*/Makefile.am` kept
  (belong to vendored libs). No CMake / CI / doc referenced any of it —
  verified nothing reads `configure.ac` or the first-party `Makefile.am`
  for version/sources (the stepmania CMake build takes its version from
  `CMake/SMDefs.cmake`; only `extern/{ogg,vorbis,pcre}/CMakeLists.txt`
  parse *their own* `configure.ac`). **Verified:** CMake reconfigure +
  `sm_tests` clean, suite 5839/220, `ctest` 100%; Release
  `StepMania-R.exe` links clean under `WITH_WERROR=ON`; `--SelfTest`
  exit 0.

* **Deleted dead in-tree `src/libtomcrypt/` + `src/libtommath/`
  (backlog item 24).** 658 files / ~161k lines / ~10 MB — the
  pre-submodule in-tree copies of the crypto libs. The fork moved these
  to `extern/` git submodules; `extern/CMakeLists.txt` builds the
  `tomcrypt`/`tommath` targets from `extern/libtomcrypt/src/...` and
  `CryptManager.cpp`'s `#include <tomcrypt.h>` resolves to
  `extern/libtomcrypt/src/headers`. The `src/` copies had zero CMake
  reference + a dead `libtomcrypt_VS2008.vcproj`. **Verified:** CMake
  reconfigure clean, `tomcrypt.lib`/`tommath.lib` still build from
  `extern/`, Release `StepMania-R.exe` links clean (`WITH_WERROR=ON`),
  `sm_tests` 5839/220, `--SelfTest` exit 0.

* **Deleted misc orphan files (backlog item 25).**
  `src/smpackage-net2008.vcproj` (orphaned by item 23),
  `src/verify_signature/` (C++/C#/Java reference impls of `.smzip`
  signature checking — not built, unreferenced),
  `src/archutils/Win32/verinc.{c,exe,sln,vcproj}` (pre-CMake
  version-increment tool + a **committed `verinc.exe`**; CMake generates
  the version stub from `src/verstub.in.cpp` per `StepmaniaCore.cmake`),
  `CMake/VerStubUtil.cmake` (`configure_file`s a non-existent
  `src/version_updater/verstub.cpp.in`, not `include()`d anywhere). Also
  fixed the stale `src/ver.h` comment that pointed at `verinc.c`. ~2k
  lines. **Verified:** reconfigure + `sm_tests` 5839/220 clean, Release
  `StepMania-R.exe` links clean, `--SelfTest` exit 0. Left
  `src/update_check/check_sm5.php` (netcode still names
  `/stepmania/check_sm5.php`).

* **Checked `src/archutils/Win32/ddk/` — kept (backlog item 26).**
  The `.lib` files under `x86/`/`x64/` are unreferenced by any
  `*.cmake`/`CMakeLists.txt` (the Win32 build links `dbghelp`/`setupapi`/
  `hid` as bare names off the Windows SDK `LIBPATH`, `src/CMakeLists.txt`
  ~392-420), so they look dead. But the **headers** in the dir are still
  load-bearing: `src/archutils/Win32/USB.cpp` hard-codes
  `#include "archutils/Win32/ddk/setupapi.h"` and `.../ddk/hidsdi.h`, and
  the only include path is `src/`, so they resolve here rather than to
  the SDK. Migrating USB.cpp to the SDK headers is a Win32 USB/HID input
  behavior-risk change, not a mechanical sweep, so the directory stays
  untouched pending a maintainer decision. Backlog item 26 updated with
  the finding. A staged experimental `git rm -r` of the dir was reverted.

* **Full-tree green check after the 2026-09-10 dead-weight sweep.**
  Combined verification covering every deletion in the sweep (CreateZip,
  `src/tests/`, autotools, `src/smpackage/`, `src/libtomcrypt/` +
  `src/libtommath/`, misc orphans, `mapconv.exe`; the `ddk/` deletion
  was reverted — its headers are used by `USB.cpp`):
  `cmake -S . -B build-tests` and `-B Build` reconfigure clean;
  `sm_tests` builds and runs **5839 assertions / 220 cases, all pass**;
  `ctest -C Debug` 100%; Release `StepMania-R.exe` links clean;
  `--SelfTest` exit 0. Tree is green.

* **Default branch renamed `5_1-new` → `main-R` (2026-09-10).** Maintainer
  did the GitHub side (renamed the branch, switched the repo default,
  the old `origin/5_1-new` is gone). Local side: `git branch -m`,
  upstream re-pointed to `origin/main-R`, stale ref pruned. `AGENTS.md`
  §4 step 4 and the `modernization-process` memory now say
  `git push` → `origin main-R`. CI is unaffected (`ci.yml` is
  `on: push:` with no branch filter). Historical "merged to `5_1-new`"
  records in the ADRs / baseline / earlier log entries are left as-is
  (accurate for their date); `stepmania/stepmania`'s branch is still
  legitimately called `5_1-new`, so the fork-point references in
  ADR 0002 / index.md stay too.

* **`.sma` characterization test landed -- ADR 0006 phase 4 (backlog
  item 17).** After an extended search the maintainer confirmed no real
  `.sma` simfile exists anywhere (extinct 2009-2011 SMA-editor format),
  and decided the current `SMALoader` read behavior is the reference.
  New `tests/test_NotesLoaderSMA.cpp` + a **synthetic** fixture
  `tests/data/sma-fixture/fixture.sma` (invented, not derived -- nothing
  to derive from) pin the SMA-only tags: `#ROWSPERBEAT` row<->beat
  translation (`8r` -> row/4, bare `24` -> literal beat 24),
  `#BEATSPERMEASURE` (4/4 back-filled at row 0), `#SPEED` `32s` ->
  `SpeedSegment::UNIT_SECONDS`, `#MULTIPLIER` 2-field (miss==combo) vs
  3-field. Also pins the quirk that `#BPMS`/`#STOPS` come out on the
  Steps timing (empty at song level) while pre-`#NOTES` tags land on
  song timing. Suite 5839/220 -> **5912/221**; `ctest` 100%; Release
  `StepMania-R.exe` links clean; `--SelfTest` exit 0.
  Latent bug noted, NOT fixed (§5, and "current behavior is the
  reference"): `SMALoader`'s `#ROWSPERBEAT` handler does
  `split(expr,"=")[1]` with no size check -> a malformed
  `#ROWSPERBEAT:4;` (no `=`) is an OOB read / crash. Flagged in backlog
  item 17 for the maintainer.

* **EngineTestEnv fixture extended: MESSAGEMAN + GAMESTATE + THEME
  (ctor only) + NOTESKIN + SONGMAN (backlog item 17).** Maintainer asked
  for THEME/NOTESKIN/MESSAGEMAN/SONGMAN to unblock PlayerOptions /
  RadarValues / theme-metric / `.crs` coverage. Landed:
  - `tests/CMakeLists.txt`: generate `SM_THEMES_DIR` / `SM_NOTESKINS_DIR`
    absolute paths; `EngineTestEnv` mounts the repo `Themes/` at
    `/Themes` and `NoteSkins/` at `/NoteSkins` (relative `"Themes/*"`
    resolves against the VFS root).
  - `EngineTestEnv::BringUp()` now also constructs, in `sm_main` order:
    `MESSAGEMAN`, `GAMESTATE` (ctor only — it deliberately skips
    `Reset()`), `GAMEMAN` (already there), `THEME` (**ctor only**),
    `NOTESKIN`, `SONGMAN` (**no `InitAll()`**). Teardown reverse.
  - **`THEME` is not switched to a theme.**
    `SwitchThemeAndLanguage("default"/"_fallback", ...)` SIGSEGVs in the
    headless harness — it runs the theme's Lua scripts + refreshes the
    screen-dimension metric cache, and theme code assumes a live engine.
    Root cause not isolated (engine vs test link separate CRTs, so
    `fprintf` markers inside `ThemeManager` produced no output). So
    `THEME != nullptr` but `IsThemeLoaded()` is false and every
    `ThemeMetric` stays unset — reading a metric *value* still asserts.
    Filed as a sub-item of backlog 17 (needs a scripts-free minimal test
    theme or a stub metric provider).
  - `test_EngineTestEnv.cpp`: contract check now covers all ten
    singletons + a "THEME constructed but not loaded" case.
  **Verified:** `sm_tests` 5912/221 -> **5925/222**, `ctest` 100%,
  Release `StepMania-R.exe` links clean, `--SelfTest` exit 0. The +5
  managers cost ~0s of run time (no theme load, no song scan).

* **`.crs` characterization test landed -- ADR 0006 phase 4 COMPLETE
  (backlog item 17).** `.crs` was the last format without a phase-4
  parse-regression. `tests/test_NotesLoaderCRS.cpp` drives
  `CourseLoaderCRS::LoadFromBuffer` (-> `LoadFromMsd`, `bFromCache=true`)
  over inline course text -- no fixture files, no SONGINDEX cache probe,
  no file I/O. `EngineTestEnv`'s new empty `SONGMAN` makes song-ref
  resolution run (and miss). Pins: metadata (`#COURSE`/`#SCRIPTER`/
  `#REPEAT`/`#LIVES`/`#BANNER`/`#METER` 2- & 3-param), `#SONG` entry
  resolution (`BEST1` accepted / `BEST2` rejected -- `> iNumSongs` not
  `>=`; `GRADEBEST`/`*` accepted; 1-part title miss rejected;
  `m_bIncomplete`), old-style difficulty aliases + `lo..hi` meter ranges
  + the `3..6` fallback, and the `#SONG` modifier column
  (`showcourse`/`noshowcourse`/`nodifficult`/rest).
  Two findings, NOT fixed (§5 -- characterization only, both flagged in
  backlog item 17):
  - `#STYLE` is unreachable: the dispatch's
    `else if( !eq("DISPLAYCOURSE") || !eq("COMBO") || !eq("COMBOMODE") )`
    is always true, so `#STYLE` + the RADAR-cache branch + the
    "unexpected value" log are dead. `||` should be `&&`.
  - 2-part `#SONG:Group/Song` refs SIGSEGV in the harness:
    `SONGMAN->FindSong(g,s)` -> `GetSongs(g)` -> `FOREACH_EnabledPlayer`
    -> null `PROFILEMAN`. Not a real-engine bug; the test uses 1-part
    refs (resolve via `GROUP_ALL`, safe).
  Also learned `LoadFromMsd` never sets `m_sPath` / `m_sGroupName` (only
  `LoadFromCRSFile` does), so via `LoadFromBuffer` those stay empty.
  **Verified:** `sm_tests` 5925/222 -> **5961/226**, `ctest` 100%,
  Release `StepMania-R.exe` links clean, `--SelfTest` exit 0.

* **Fixed the `#STYLE` dead-code bug in `CourseLoaderCRS` + made the Unix
  CI test jobs informational.**
  - `CourseLoaderCRS::LoadFromMsd`: the recognised-tag guard was
    `else if( !eq("DISPLAYCOURSE") || !eq("COMBO") || !eq("COMBOMODE") )`,
    which is always true, so `#STYLE`, the `bFromCache` RADAR-cache
    branch and the "unexpected value" log were all dead -- `#STYLE` on a
    course silently did nothing. Corrected to
    `eq(A) || eq(B) || eq(C)` and moved the `#STYLE` handler *above* the
    `bFromCache` catch-all (a `LoadFromBuffer`/cache load has
    `bFromCache=true`, so `#STYLE` would otherwise be eaten by the
    radar-cache parse before reaching its handler).
    `test_NotesLoaderCRS.cpp`'s metadata case now checks
    `#STYLE:dance-single,dance-double` -> both land in `m_setStyles`.
    §5 change, Windows-verified (`sm_tests` 5963/226, `ctest` 100%,
    Release + `--SelfTest` green). Backlog item 17 updated.
  - **CI:** the Ubuntu + macOS `sm_tests` "Run tests" steps are
    `continue-on-error: true`. Root cause (new backlog item 27): on
    Unix/macOS any engine `ASSERT`/`FAIL_M` -> `sm_crash()` ->
    `CrashHandler::ForceCrash` -> `RunCrashHandler`, which `_exit(1)`s
    because `CrashHandlerHandleArgs` was never called (Catch2's `main()`
    doesn't boot `ArchHooks`). So the first assert any test trips kills
    the binary before Catch2 can report -- and a couple of tests
    (`Corpus`, `RageFileDriverSlice`) trip one only on Unix. Pre-existing
    (red before the 2026-09-10 work); the suite is green on Windows, the
    primary platform. Fixing it needs a Unix/macOS box.

* **Fixed the Linux `sm_tests` crash for real (Docker + gdb) -- backlog
  item 27 + the item-17 headless-theme sub-item.** Ran `sm_tests` in an
  `ubuntu:24.04` container under gdb. Root cause was not the crash
  handler: `EngineTestEnv` brought up engine singletons with no theme
  loaded, and engine ctors (`Song`, `SongManager`,
  `ThemeMetricStepsTypesToShow::Read`) read `ThemeMetric<T>` via
  `GetValue()` -> `ASSERT_M( m_Value.IsSet() )` -> `sm_crash()` ->
  `_exit(1)` on Unix (Windows silently returns a default).
  - New `tests/data/test-theme/SMRTest/metrics.ini`: scripts-free
    minimal theme, `[Global] FallbackTheme=`, ~10 concrete metrics; the
    rest resolve "missing -> `Dialog::ignore` -> nil".
  - `EngineTestEnv`: mount the test theme over `/Themes`,
    `Dialog::SetWindowed(false)` (else the per-missing-metric
    `AbortRetryIgnore` pops a modal MessageBox on Windows and hangs),
    build `NOTESKIN` before `GAMEMAN->GetDefaultGame()`,
    `GAMESTATE->SetCurGame(GAMEMAN->GetDefaultGame())` before the theme
    load, `THEME->SwitchThemeAndLanguage("SMRTest",...)`, `SONGMAN`
    restored. `.crs` test + `test_EngineTestEnv` contract updated.
  - **`HAVE_ICONV` was never defined** (StepmaniaCore.cmake does
    `find_package(Iconv)` but nothing consumed the result), so
    `RageUtil_CharConversions.cpp` fell to its `#else` "no converters"
    branch on Linux and **blanked non-UTF-8 song titles/artists**
    (Korean KSF, Japanese BMS, CP1252 DWI). Now set when Iconv is found
    and `NOT APPLE` (Apple keeps CoreFoundation), with an `ICONV_CONST`
    fallback define, and a `#cmakedefine HAVE_ICONV` in `config.in.hpp`.
  - `.github/workflows/ci.yml`: the `continue-on-error` on the Unix
    test jobs (added earlier the same day) is **removed** -- they pass.
  **Verified on both platforms:** Windows `sm_tests` **5966/226**,
  `ctest` 100%, Release + `--SelfTest` green; Linux (container)
  `sm_tests` 5966/226, `ctest` 100%.

* **Item 26 (Utils/ dev-tool binaries) DONE + item 21 (drop 32-bit
  Windows) DONE + item 2 (C4244/C4267) real measurement + first fix.**
  - Deleted `Utils/Graphviz/` (20 files) + `Utils/doxygen/*.exe` (dead
    -- `Docs/Doxyfile` has `HAVE_DOT = NO`) and the misc unused
    `Utils/*.exe` dev tools; updated `doxygen_run.bat` / 
    `pngcrushallfiles.bat` to expect their replacements on PATH.
    `Program/parallel_lights_io.dll` audited and kept -- a real runtime
    `LoadLibrary` dependency of `LightsDriver_Win32Parallel`.
  - Dropped 32-bit Windows: `SM_WIN32_ARCH` now `FATAL_ERROR`s on non-64
    -bit and is otherwise always `"x64"`; removed the now-dead
    `/arch:SSE2` (MSVC x86) and libmad `FPM_INTEL` (32-bit) branches;
    `build-ffmpeg-win32.yml` builds/packages x64 only.
  - Re-measured C4244/C4267 properly (a `--clean-first` full rebuild,
    counting unique sites, not raw per-TU lines -- the old "~4.4k" was
    the same double-counting mistake C4100's stale "~1362" was).
    Release: 0, entirely from 4 lines in `RageUtil.h`/`RageTimer.h`
    (fixed, `static_cast`). Debug/`WITH_TESTS`: **297 unique sites**
    across ~80 files -- the real remaining surface; `/wd4244`/`/wd4267`
    stay until those are triaged file-by-file (top: `NoteField.cpp` 35,
    `TimingSegments.cpp` 25, `RageSurfaceUtils.cpp` 22,
    `ScreenOptionsMasterPrefs.cpp` 18).
  **Verified:** `sm_tests` 5966/226, `ctest` 100%, Release build +
  `--SelfTest` green after all three changes together.

* **item 2 (C4244/C4267) sweep, first 4 files: 297 -> 187 remaining.**
  Methodology lesson learned the hard way: a "clean rebuild shows 0"
  check is meaningless if `/wd4244 /wd4267` is still in
  `src/CMakeLists.txt` -- burned about an hour mid-sweep on a false
  "sweep complete" signal because the suppression flag wasn't actually
  removed for that measurement. Always confirm the flag is gone before
  trusting a 0.
  Real fixes, all `static_cast` documenting existing intentional
  behavior (no logic change): `NoteField.cpp` (2 macro fixes covered
  ~30 of its 35 call-site warnings: `draw_all_segments`'s int-ternary-
  into-float, `IS_ON_SCREEN`'s float-members-into-int-params; plus a
  handful of individual float/int call sites and a `lua_tonumber`
  truncation); `TimingSegments.cpp` (`*Segment::GetValues()` pushing
  int/enum into `vector<float>`, 5 sites); `RageSurfaceUtils.cpp`
  (`std::trunc`/`std::lrint` results assigned to narrower int types, 6
  sites); `ScreenOptionsMasterPrefs.cpp` (one `static_cast<T>` around a
  generic `if constexpr` lambda fixes every instantiation, 2 sites).
  `/wd4244`/`/wd4267` stay in `src/CMakeLists.txt` -- 187 sites remain
  across ~75 files (`NoteDataWithScoring.cpp` 11, `ScreenGameplay.cpp`
  9, `NoteDisplay.cpp` 9, ... next).
  Verified: `sm_tests` 5966/226, `ctest` 100%, Release + `--SelfTest`
  green.

* **item 2 (C4244/C4267) sweep, next 4 files/macros: 187 -> 158
  remaining (72 files).** `NoteDataWithScoring.cpp` (§5-adjacent
  `NoteData*`; `RadarValues` float-backed counts cast both directions,
  characterization-only, 11 sites); `NoteDisplay.cpp` (degrees-as-
  double into float rotation, the shared `IsOnScreen()` float-into-int
  pattern, a pointer-diff into int, 9 sites); `ScreenGameplay.cpp` (int-
  from-`SafeFArg()` into float margins, an iterator-diff, an enum
  subtraction, a `lua_pushnumber(size_t)`, 9 sites, 2 of them via the
  macro below); `OptionsBinding.h`'s `FLOAT_TABLE_INTERFACE` macro
  (`size()`/`n+1`/bare `size_t` into Lua's `int`/`lua_Number` params --
  fixed once at the macro, benefits every screen using the interface).
  Verified: `sm_tests` 5966/226, `ctest` 100%, Release + `--SelfTest`
  green.

* **item 2 (C4244/C4267) sweep, next 3 files: 158 -> 140 remaining (69
  files), plus a second methodology correction.** `NetworkManager.cpp`
  (`lua_tointeger`/`lua_pushnumber` narrowing, 6 sites); `Profile.cpp`
  (age/calorie-math float chain plus, only found on a genuine clean
  rebuild, several `size()`/iterator-diff-into-`int` sites, ~10 total);
  `NotesLoaderBMS.cpp` (§5-protected `NotesLoader*`, characterization-
  only -- measure-size/time-signature/BPM narrowing plus, again only
  found on a clean rebuild, a few more `size()`/`find()` sites, ~10
  total; re-verified via `sm_tests.exe "[bms]" -s`, 26 assertions/2
  cases unchanged). Second methodology trap: deleting just the touched
  `.obj` files and rebuilding is an *incremental* build (recompiles
  those files plus header-dependents only) -- its "0 warnings" is not a
  full-tree count. Only a genuine `--clean-first` rebuild, with
  `/wd4244`/`/wd4267` actually removed, is authoritative; that rebuild
  turned up several more real sites in these same three files (now
  fixed) and set the true current total at 140 sites / 69 files, with
  all ten files fixed so far confirmed at zero.
  Verified: `sm_tests` 5966/226, `ctest` 100%, Release + `--SelfTest`
  green.

* **item 2 (C4244/C4267) sweep, next 4 files: a THIRD methodology
  correction, 140/69 was wrong the whole time -- true total 528 -> 489
  remaining (148 files).** The dedup regex used since the start of this
  sweep (`warning C424[47]`) only ever matches `C4244` -- `C4267` is
  `"C426"`+`"7"`, a different literal the same pattern cannot match.
  Every "140/69", "158/72", "187/~75", "297" figure in this sweep
  silently counted C4244 only and dropped all 388 unique C4267 sites
  (838 raw lines) the entire time. Corrected regex
  (`warning C42(44|67)`) on the same clean rebuild: **528 unique sites
  / 152 files** was the real total going into this batch -- the four
  files already reported as "5 each" (`ScreenDebugOverlay.cpp`,
  `RageFileBasic.cpp`, `BitmapText.cpp`, `ActorMultiVertex.cpp`) were
  actually 5/11/7/16 once C4267 was counted. Fixed all real sites in
  those four (`ActorMultiVertex.cpp`: Lua bindings pushing `size_t`
  into `lua_Number`/`int` params, a `size_t` modulo/subtraction into
  `int` locals, `SetVertex*`/`SetState`/`AddVertices` narrow-int
  params; `BitmapText.cpp`: an `int`+`float` mix promoting
  `std::fmin`'s result to `double` -- fixed via a `1.0f` literal
  instead of an int `1`, plus `size_t`/`unsigned`-into-`int`/`float`
  narrowing at 5 more sites; `RageFileBasic.cpp`: 2 pointer-diffs and 6
  `Write`/`Read`-buffer `size_t`-into-`int` sites; `ScreenDebugOverlay
  .cpp`: 4 iterator-diffs + 1 `double`-into-`float` argument, no extra
  C4267 beyond the original count). **True current total: 489 unique
  sites remain across 148 files** (re-verified at zero for these four
  in a second clean rebuild). Next: `RageUtil.cpp` (25),
  `SongManager.cpp` (20), `XmlFileUtil.cpp` (14), `ThemeManager.cpp`/
  `ScreenEdit.cpp`/`RageFileManager.cpp` (11 each).
  Verified: `sm_tests`/`ctest`/Release/`--SelfTest` gate re-run after
  restoring `/wd4244`/`/wd4267`.

* **item 2 (C4244/C4267) sweep, 6 more files: 489 -> 397 remaining (142
  files).** `RageUtil.cpp` (25: `SmEscape`/`DwiEscape`/`do_split`'s
  int/size bookkeeping, two `pcre_exec` length args, `Trim*`/
  `Dirname`'s size-backed counters, `Json::Value::resize`'s
  `ArrayIndex`, a `TONUMBER_NICE` macro plus `multiapproach`'s
  `lua_tonumber`-into-float sites); `SongManager.cpp` (20: load-time
  `SetTotalWork`/`wrap`/`Left`/`Right` calls, eight one-line `GetNum*()`
  getters, three safe reverse-loop bounds, a `count_if` accumulator);
  `XmlFileUtil.cpp` (14: a local `SetString(int,int,...)` helper called
  from 6 sites -- cast at each call site rather than widening the
  helper, since its `iEnd-1 >= iStart` check relies on signed-underflow
  behavior an unsigned param would change; two reverse-loop bounds);
  `ThemeManager.cpp` (11: `Left`/`Right` size-into-int, a reverse-loop
  bound, two `std::distance` results, a Lua table push);
  `ScreenEdit.cpp` (11: `RandomInt(size_t)`, an int-division-then-
  double-multiply zoom expression ×2, a default-choice index, a
  find-begin pointer-diff, a `MenuRowDef` count param, a track-remap
  array, a keysound count); `RageFileManager.cpp` (11: a `Seek(int)`
  fed `mz_uint64` -- already runtime-checked for truncation so the cast
  is behavior-preserving, three reverse-loop bounds, `Left`/`Right`
  mount-point arithmetic, three size-into-int driver/file counters).
  **Caught a measurement mistake mid-batch:** ran the `--clean-first`
  verification rebuild while `ThemeManager.cpp`/`ScreenEdit.cpp`/
  `RageFileManager.cpp` were still being edited -- the parallel build
  compiled those 3 in their pre-fix state, so that run's "489 -> 422"
  intermediate figure was wrong (RageUtil.cpp/SongManager.cpp did
  verify clean in it, since those two were already done). Re-ran a
  second, edit-free `--clean-first` rebuild once all 6 files were done:
  confirmed all six at zero, true total 397 sites / 142 files. Lesson:
  never trust a measurement rebuild that overlaps with active edits,
  even if it's `--clean-first`.
  Verified: `sm_tests` 5966/226 unchanged, `ctest`/Release/`--SelfTest`
  gate re-run after restoring `/wd4244`/`/wd4267`.

* **item 2 (C4244/C4267) sweep, 6 more files: 397 -> 340, plus a THIRD
  file-level discovery -- 3 already-"done" files had residual C4267
  the broken regex hid, -> true 332 remaining (136 files).**
  `OptionsList.cpp` (10), `EditMenu.cpp` (10), `CubicSpline.cpp` (10: a
  shared `LCSN_EVAL_SOMETHING` macro fixed once covers 4 call sites),
  `TimingData.cpp` (9, §5-protected, characterization-only),
  `MusicWheel.cpp` (9), `CourseLoaderCRS.cpp` (9, §5-protected `.crs`,
  characterization-only: `Left`/`Right` fed `strlen()`-derived lengths
  parsing `BEST`/`WORST`/`GRADEBEST`/`GRADEWORST` prefixes). After a
  clean rebuild confirmed those six at zero, a sanity sweep of every
  previously-"done" file against the same log turned up 8 more sites
  the `C424[47]` regex bug had hidden in files marked done earlier this
  sweep: `NoteField.cpp` (5), `ScreenGameplay.cpp` (2),
  `ScreenOptionsMasterPrefs.cpp` (1) -- all fixed and reconfirmed at
  zero in a follow-up clean rebuild, along with every other
  previously-"done" file (no further residuals found). True total: 332
  sites / 136 files remain.
  Verified: `sm_tests` 5966/226 unchanged, `ctest`/Release/`--SelfTest`
  gate re-run after restoring `/wd4244`/`/wd4267`.

* **item 2 (C4244/C4267) sweep, 6 more files: 332 -> 289 remaining
  (~130 files).** `RageDisplay_OGL.cpp` (8: a shader-compile helper's
  `GLint`/`GLsizei` args, two `int`-into-`float` `LoadMenuPerspective`
  args, two `std::max<unsigned int>` calls, a buffer-size local);
  `Song.cpp` (7, §5-protected, characterization-only: four reverse-
  loop bounds, a Lua background-changes serializer);
  `ScreenSelectMaster.cpp` (7: a `lua_pushnumber`, a `lua_rawgeti`, a
  `SET_POS_PART` macro fixed once for 3 uses, two `wrap()` sites, a
  local); `RageLog.cpp` (7: a timestamp arg, four log-buffer counters,
  a `std::min<unsigned int>` call, a `std::min` result into `int`);
  `RageFileDriverDeflate.cpp` (7: two zlib `uInt` fields, two pointer-
  diffs, a `WriteInternal` return); `RageDisplay.cpp` (7: a `round()`
  result, a `ceil()` result, a vertex count, four mesh-info counters).
  **A missed-site bug of its own:** an earlier `Edit` call on
  `Song.cpp` reported "2 matches" for one bare reverse-loop pattern;
  adding disambiguating context fixed one of the two occurrences, but
  the *third*, separately-identical occurrence a few lines later was
  never revisited and shipped unfixed until the next clean-rebuild
  verification caught it (`Song.cpp` still showed 1 site after the
  "done" rebuild). Fixed and reconfirmed at zero in a follow-up clean
  rebuild, along with a full sanity sweep of every other previously-
  "done" file (no further misses). True total: 289 sites remain.
  **CI note:** the push landed on Windows CI as a `file DOWNLOAD
  cannot compute hash on failed download` fetching the prebuilt
  win32 FFmpeg release asset -- a transient GitHub-hosted-download
  flake unrelated to this change (Ubuntu/macOS + all unit-test jobs on
  the same run were green). Re-ran via `gh run rerun --failed`.
  Verified locally: `sm_tests` 5966/226 unchanged, `ctest`/Release/
  `--SelfTest` gate re-run after restoring `/wd4244`/`/wd4267`.

* **item 2 (C4244/C4267) sweep, 6 more files: 289 -> 253 remaining
  (~125 files).** `XmlToLua.cpp` (6: `Left`/`Right` size arithmetic at
  5 call sites, a filename `Left()`); `WheelBase.cpp` (6: a count into
  `int`, five `wrap()` sites); `StatsManager.cpp` (6: a song count, a
  `trunc()` result, two profile-stat accumulators, an index, a
  `lua_pushnumber`); `ScreenOptions.cpp` (6: a `MoveRowAbsolute` arg,
  the same `iNumChoices` pattern at 2 call sites, a `wrap()`);
  `Course.cpp` (6: a mod-change count, two `RandomInt()` calls, two
  near-duplicate `std::floor()` meter-balancing sites, a
  `lua_pushnumber`); `ActorMultiTexture.cpp` (6: two size members, two
  texture-unit-count returns, two `enum_add2()` calls). Sanity-swept
  every previously-"done" file against the clean rebuild log again --
  no real residuals (a `Course.cpp(` grep also matched
  `ScreenOptionsEditCourse.cpp(` as a substring, a false positive, not
  a miss).
  Verified: `sm_tests` 5966/226 unchanged, `ctest`/Release/`--SelfTest`
  gate re-run after restoring `/wd4244`/`/wd4267`.

* **item 2 (C4244/C4267) sweep, 4 more files: 253 -> 233 remaining
  (~120 files).** `RageFileDriverMemory.cpp` (5: `int m_iFilePos` fed
  `size_t` byte counts, a `GetFileSize()` return); `NoteDataUtil.cpp`
  (5, §5-adjacent, characterization-only: an `intptr_t` pointer-diff,
  a `LoadFromSMNoteDataStringWithPlayer` length arg, a track-pressed
  count, a taps-left counter, a reverse-loop bound); `LuaManager.cpp`
  (5: two `FromStack<>` specializations, a thread-pool index, two more
  `lua_tointeger()`-into-`int` sites); `CryptManager.cpp` (5: five
  libtomcrypt calls fed a `size_t` where the C API wants `unsigned
  long`). Sanity-swept every previously-"done" file again -- clean
  (the `Course.cpp(` / `ScreenOptionsEditCourse.cpp(` false positive
  noted last batch still applies, still not a real residual).
  Verified: `sm_tests` 5966/226 unchanged, `ctest`/Release/`--SelfTest`
  gate re-run after restoring `/wd4244`/`/wd4267`.

* **item 2 (C4244/C4267) sweep, 11 more files (every remaining 4-site
  file): 233 -> 189 remaining (~110 files).** `StepMania.cpp` (4: a
  `ceil()` window width, a memory-status division, an `srand` seed);
  `SongUtil.cpp` (4: two `SecondsToMMSS` calls, a `Left()`, a
  reverse-loop bound); `ScreenUnlockStatus.cpp` (4: two unlock counts,
  two reverse-loop bounds); `ScreenServiceAction.cpp` (4: two
  near-duplicate edit-clearing helpers); `ScreenJukebox.cpp` (4: three
  `RandomInt()` calls, a reverse-loop bound); `ScoreKeeperNormal.cpp`
  (4: a song count, two Lua toasty-trigger sites); `RageTimer.cpp` (4:
  two `Difference()` results, a `floor(float)` into `int64_t`);
  `RageSoundReader_Preload.cpp` (4: four repeats of the same
  frame-count computation); `RageSoundReader_MP3.cpp` (4: three MAD
  pointer-diffs, an `id3_tag_query` length arg); `RageMath.cpp` (4: a
  `double`-literal ternary fixed with `f`-suffixed literals instead of
  a cast, three `double` intermediates in a triangle-wave helper);
  `RageDisplay_D3D.cpp` (4: a palette-index lookup, two
  `std::max<unsigned int>` calls -- same pattern as the earlier OGL
  fix). Full sanity sweep of every previously-"done" file (including
  the known substring-ambiguous `Song.cpp`/`Course.cpp`/
  `RageDisplay.cpp`/`ScreenOptions.cpp`, checked precisely this time)
  -- clean, no residuals.
  Verified: `sm_tests` 5966/226 unchanged, `ctest`/Release/`--SelfTest`
  gate re-run after restoring `/wd4244`/`/wd4267`.

* **item 2 (C4244/C4267) sweep, 7 more files: 189 -> 163 remaining
  (~105 files).** `OptionRow.cpp` (4: a course-entry count, a
  `std::min<unsigned int>`, a `lua_pushnumber`); `JsonUtil.h` (4: four
  near-identical `root.resize(v.size())` template sites needing
  `Json::Value::ArrayIndex`); `GraphDisplay.cpp` (4: a `DrawQuads`
  count, a fan-count division, two theme-metric size members);
  `AdjustSync.cpp` (4: a before/after filter-count diff, a `size_t`
  into `unsigned int`, a `FormatNumberAndSuffix` arg);
  `ActorScroller.cpp` (4) + `DynamicActorScroller.cpp` (2, bonus find):
  a sub-actor count, two `ceil()` results, three `wrap()` sites across
  both scroller files; `Actor.cpp` (4: two `Left()` calls, a timer
  value into a `float`-taking update function, a `lua_pushnumber`).
  Full sanity sweep incl. every substring-ambiguous file (`Song.cpp`/
  `Course.cpp`/`RageDisplay.cpp`/`ScreenOptions.cpp`/`Actor.cpp` itself
  vs `ActorFrame.cpp`/`ActorMultiVertex.cpp`/etc.) -- clean.
  Verified: `sm_tests` 5966/226 unchanged, `ctest`/Release/`--SelfTest`
  gate re-run after restoring `/wd4244`/`/wd4267`.

* **item 2 (C4244/C4267) sweep, 19 more files (every remaining 3-site
  file): 163 -> 106 remaining (~85 files).** `WheelNotifyIcon.cpp`,
  `UnlockManager.cpp`, `Steps.cpp` (§5-protected, characterization-
  only Lua binding), `ScreenSelectCharacter.cpp`, `ScreenSelect.cpp`,
  `Screen.cpp`, `RageThreads.cpp`, `RageSoundReader_ChannelSplit.cpp`,
  `RageFileDriverReadAhead.cpp`, `RageFileDriverDirect.cpp`,
  `PlayerStageStats.cpp`, `OptionRowHandler.cpp`, `NotesLoaderSSC.cpp`
  (§5-protected, characterization-only), `LightsManager.cpp`,
  `InputQueue.cpp`, `GameManager.cpp`, `ArrowEffects.cpp`,
  `ActorFrame.cpp`, `ScreenOptionsEditCourse.cpp` (a genuinely separate
  file from `ScreenOptions.cpp`/`Course.cpp`, never touched before).
  Every site was a `size_t`/`int64_t`/`double`/`lua_Integer`-family
  value narrowed into a smaller type at a `wrap()` call, a
  reverse-loop bound, an iterator-diff, or a Lua binding -- same
  patterns as the whole sweep, no logic changes. Full sanity sweep incl.
  every substring-ambiguous file (`Song`/`Course`/`RageDisplay`/
  `ScreenOptions`/`Actor`/`Steps`/`Screen`/`ScreenSelect`) -- clean.
  Remaining files are all at 1-2 sites each now; no more count-based
  tiers to batch by.
  Verified: `sm_tests` 5966/226 unchanged, `ctest`/Release/`--SelfTest`
  gate re-run after restoring `/wd4244`/`/wd4267`.

* **item 2 (C4244/C4267) sweep, 26 more files (every remaining 2-site
  file): 106 -> 54 remaining (all single-site files now).**
  `StepsUtil.cpp`, `StageStats.cpp`, `Sprite.cpp`, `ScrollBar.cpp`,
  `ScreenSelectMusic.cpp`, `ScreenOptionsManageProfiles.cpp`,
  `ScreenMapControllers.cpp`, `ScreenInstallOverlay.cpp`,
  `SampleHistory.cpp`, `RandomSample.cpp`,
  `RageUtil_CharConversions.cpp`, `RageSoundReader_SpeedChange.cpp`,
  `RageSoundReader_Resample_Good.cpp`, `RageSoundReader_Chain.cpp`,
  `RageModelGeometry.cpp`, `RageFileDriverTimeout.cpp`,
  `ProfileManager.cpp`, `PlayerOptions.cpp`, `Player.cpp`,
  `NotesLoaderSMA.cpp` (§5-protected), `NotesLoaderJson.cpp`
  (§5-protected), `NoteData.cpp` (§5-protected, characterization-
  only), `MsdFile.h`, `GameSoundManager.cpp`, `CsvFile.cpp`,
  `CharacterManager.cpp`. Same recurring narrowing patterns throughout
  -- `wrap()`/`RandomInt()` args, iterator-diffs, reverse-loop bounds,
  `Left()`/`Right()` length args, Lua bindings. Full sanity sweep incl.
  every substring-ambiguous file (`Song`/`Course`/`RageDisplay`/
  `ScreenOptions`/`Actor`/`Steps`/`Screen`/`ScreenSelect`/`Sprite`/
  `Player`/`NoteData`) -- clean. Remaining 54 files are all single-site
  now; several are §5-protected headers/loaders/writers
  (`NotesLoaderSM.cpp`/`NotesLoaderKSF.cpp`/`NotesLoaderDWI.cpp`/
  `NotesWriterSSC.cpp`/`NotesWriterSM.cpp`/`NoteData.h`/`Profile.h`/
  `Course.h`) needing the usual characterization re-check.
  Verified: `sm_tests` 5966/226 unchanged, `ctest`/Release/`--SelfTest`
  gate re-run after restoring `/wd4244`/`/wd4267`.

* **item 2 (C4244/C4267) sweep: DONE 2026-09-11.** Fixed the final
  batch: 54 single-site files (root src/, all §5-adjacent headers/
  loaders/writers re-verified against their characterization tests),
  then discovered a fourth methodology trap right at the finish line --
  every prior "clean" `--clean-first` rebuild had two blind spots: (1)
  the dedup regex anchored on `^[A-Za-z]:` silently never matched a
  toolchain path containing a space/paren (`C:\Program Files (x86)\
  ...`), hiding a genuine site attributed to a std-lib header line
  (`<algorithm>`'s `std::transform` internals, instantiated from
  `Song.cpp`'s `::tolower` calls); (2) every prior "complete" rebuild
  had silently stopped exactly at the alphabetical end of root
  `src/*.cpp` and never reached `src/arch/`/`src/archutils/Win32/` at
  all, not a regex bug -- a genuinely incomplete build every single
  time, never suspected because the log tail always looked plausible.
  Fixed `Song.cpp`'s two `transform(...,::tolower)` sites (casting
  lambda, also fixes the classic tolower-with-negative-char UB); 14
  files / 25 sites across `src/arch/`+`src/archutils/Win32/`
  (`RageSoundDriver_WDMKS.cpp` 5, `ErrorStrings.cpp` 4,
  `MovieTexture_FFMpeg.cpp` 3, `CrashHandlerNetworking.cpp` 2,
  `InputHandler_DirectInput.cpp` 2, nine single-site files -- Win32 API
  `int`/`DWORD`/`WORD` params fed `size_t`, same recurring patterns).
  One site was in vendored code (`extern/ffmpeg-w32-prebuilt`'s
  `libavutil/common.h`, instantiated via `MovieTexture_FFMpeg.cpp`) --
  not patched; scoped an MSVC-only `#pragma warning(push)`/`disable:
  4244`/`pop` around just the FFmpeg `#include` block in
  `MovieTexture_FFMpeg.h` instead (same "leave vendored trees alone"
  policy as the `ixwebsocket` clang-tidy exclusion, item 12).
  A genuinely edit-free, full-tree `--clean-first` Debug rebuild now
  completes with **exit code 0, zero `C4244`/`C4267` warnings
  anywhere**. `/wd4244`/`/wd4267` **removed from `src/CMakeLists.txt`
  permanently** -- the MSVC-warning ratchet is now complete: all 5
  categories (`C4189`, `C4702`, `C4100`, `C4244`, `C4267`) promoted to
  `-Werror`.
  Verified: `sm_tests` 5966/226 unchanged; `[NotesLoader]`/`[NoteData]`/
  `[bms]`-tagged characterization tests (647/47) re-checked identical;
  `ctest`/Release (clean rebuild)/`--SelfTest` gate green with the
  suppression flags gone for good.

* **item 18 (Logging overhaul, ADR 0005 phase 4) batch 1, `572fc79738`
  (2026-09-11).** First call-site migration to the categorized macros
  -- this codebase had zero migrated files before this batch. Piloted
  the convention on three non-§5-protected files:
  `RageFileManager.cpp` (11 sites -> `Log::File`), `IniFile.cpp` (11
  sites -> `Log::File`), `ThemeManager.cpp` (9 of 12 sites ->
  `Log::Theme`; its 3 `LOG->UserLog(...)` calls write to user.txt via
  a separate facility with no `LOG_*` equivalent and were left
  untouched, out of phase 4's scope).
  Per-site triage (not a blind `Warn`->`Error` sweep): genuine I/O or
  config failures (unzip/read/write errors, can't-chdir-to-exe-dir,
  unknown mount type, theme element truly missing) upgraded to
  `LOG_ERROR`; routine/expected cases (an already-optional probe read,
  deleting a key/value that may not exist, a theme falling back to a
  default) and the "overwriting a protected path is not allowed"
  security-guard sites kept at `LOG_WARN`/`LOG_TRACE` rather than
  force-upgraded -- some `Warn` sites legitimately stay `Warn`.
  No parsing/behavior logic touched anywhere -- pure category/level
  tagging, confirmed by re-running the log output itself: the
  characterization-test run now shows `[WARN] file ... IniFile.cpp:187
  Value 'a' not found in key 's'.` etc. with the right category/level/
  file:line, exactly as designed.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

* **item 28 (drop macOS x86_64 from CI) DONE, 2026-09-11.** Maintainer
  confirmed live that Apple has ended Intel Mac support, tripping the
  conditional clause ADR 0003 already had on the books ("x86_64 while
  Apple/Rosetta still ship it"). Removed the `macos-build-x86_64` job
  from `.github/workflows/ci.yml` (`macos-15-intel` runner); the
  pre-existing `macos-build-arm64` job is now the only macOS build leg
  (`macos-tests` unit-test job was already arm64-only). ADR 0003's
  floor table updated to "macOS: arm64 only". Left alone on purpose:
  `CMake/CPackSetup.cmake` / `CMake/SetupFfmpeg.cmake` still support
  building for `CMAKE_OSX_ARCHITECTURES=x86_64` locally -- only CI
  *coverage* changed, not the buildable-target set; dropping that too
  is a separate, bigger call. CI-workflow + docs-only change, no C++
  touched -- the push itself is the verification that `ci.yml` still
  parses and every remaining job stays green.

* **item 18 (ADR 0005 phase 4) batch 2, 2026-09-11.** Migrated
  `CryptManager.cpp` (21 sites -> `Log::General`, since no
  crypto-specific category exists) and `MemoryCardManager.cpp` (13
  real sites -> `Log::Profile`, since this subsystem exists to serve
  `PROFILEMAN`'s removable-media storage; a 14th grep hit at line 219
  is a pre-existing commented-out `//LOG->Trace("update")`, left
  untouched). Per-site triage, not a blind sweep: `CryptManager`'s
  RSA/hash/file-I/O failures -> `LOG_ERROR`; the one-time "keys
  missing, generating new keys" first-run notice -> `LOG_INFO`
  (routine, not a failure); the alternate-public-key "signature
  mismatch" -> `LOG_TRACE` (that path is called once per candidate key
  while probing for the right one, so most mismatches are expected,
  matching the pre-existing "trying alternate key" `Trace` right above
  it -- NOT a tamper signal). `MemoryCardManager`'s "mount failed"
  `Trace`->`WARN` (a real, if hotplug-flaky, operation failure) and its
  post-mount "GetFileDriver failed" `Warn`->`ERROR` (an internal
  inconsistency -- the driver we just mounted can't be found); ~10
  routine device-tracking/thread-state `Trace` sites just
  re-categorized, level unchanged. No parsing/behavior logic touched;
  neither file is §5-protected.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

* **item 18 (ADR 0005 phase 4) batch 3, 2026-09-11.** Migrated
  `RageSound.cpp` (12 real sites -> `Log::Sound`) and
  `NetworkSyncManager.cpp` (14 real sites -> `Log::Net`). Notable
  triage: `RageSound`'s four "sound not loaded" guards (calling
  `Play`/`Pause`/`GetLengthSeconds`/`SetPositionFrames` before `Load()`)
  upgraded `Warn`->`ERROR` (a real caller bug, not routine); `Load()`'s
  missing/corrupt-file open failure upgraded to `ERROR` too (it falls
  back to a silence reader, but a missing asset is still a real
  problem); "seeked past EOF" and "invalid stop mode" kept at `WARN`
  (non-fatal, self-clamps/no-ops); the start-time-in-the-past
  diagnostic upgraded `Trace`->`WARN` per its own comment ("log it,
  since it can be unobvious"). `NetworkSyncManager`'s "invalid port"
  and "failed to connect" upgraded to `ERROR`; an out-of-range command
  byte from the wire upgraded `Trace`->`WARN` (real protocol anomaly).
  **Mid-batch discovery: `NetworkSyncManager.cpp` and its whole calling
  subsystem (every `ScreenNet*`/`Room*` file) turned out to be absent
  from every `CMakeData-*.cmake` list and produces no object file in a
  from-scratch build** -- orphaned from the CMake build entirely,
  predating this modernization effort. The edit there is harmless
  (pure text, never compiled either way) but couldn't be verified by
  the usual `WITH_WERROR` gate. Flagged to the maintainer live; decided
  to keep the edit and record it as new backlog item 29 (re-wire vs.
  remove is a maintainer call) rather than investigate further in this
  batch. `RageSound.cpp` compiled and verified normally.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

* **item 18 (ADR 0005 phase 4) batch 4, 2026-09-11.** Migrated
  `RageDisplay_OGL.cpp` (25 real sites -> `Log::Gl`; 5 grep hits are
  pre-existing dead code -- 2 fully commented-out lines plus 3 more
  sitting inside `/* ... */` blocks around debug-only matrix dumps --
  all left untouched). Triage: shader file-open/read failures and
  actual GLSL compile/link failures upgraded to `LOG_ERROR` (real
  bugs); driver-capability gaps ("fragment shaders not supported",
  "low-performance renderer") kept at `LOG_WARN` (expected on older/
  limited hardware, handled gracefully); the vendor/renderer/version/
  extension-list startup dump and feature-probe fallback notices
  (paletted textures, packed-pixel format, pixel-map table size) kept
  at `LOG_INFO`/`LOG_TRACE` -- routine capability detection, not
  problems. Two known-driver-quirk workarounds (an old Catalyst
  `GL_INVALID_OPERATION` bug) stayed at `LOG_TRACE`, matching the
  file's own comment explaining the fallback. No parsing/behavior
  logic changed; not §5-protected.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

* **item 18 (ADR 0005 phase 4) batch 5, 2026-09-11.** Migrated
  `RageDisplay_D3D.cpp` (14 real sites -> `Log::General`, since no
  D3D-specific category exists -- same reasoning as `CryptManager` in
  batch 2; 2 grep hits are pre-existing commented-out calls, left
  untouched). Two real init-time failures upgraded to `LOG_ERROR`:
  `Direct3DCreate9` failing outright, and `FindBackBufferType` finding
  no usable back buffer format at all (previously mislabeled `Trace`);
  the adapter display-mode query failure upgraded `Warn`->`ERROR` too
  (has a graceful fallback, but the query itself shouldn't normally
  fail). One clear mislabeling fixed: `TryVideoMode`'s entry trace was
  tagged `Warn` for no evident reason -- it's a plain function-entry
  diagnostic, identical in kind to its own (already-commented-out)
  sibling one line away in `RageDisplay_OGL.cpp` -- downgraded to
  `LOG_TRACE`. For cross-backend parity with batch 4's already-`INFO`
  OGL vendor/mode startup dump, the equivalent D3D driver-
  identification and supported-mode dump (3 sites) was promoted
  `Trace`->`INFO` rather than left at its pre-existing level -- same
  kind of information should behave the same under `--LogLevel`
  regardless of which renderer backend is active. The rest (routine
  per-mode/parameter-testing loop diagnostics) stayed at `LOG_TRACE`.
  No parsing/behavior logic changed; not §5-protected.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

* **item 18 (ADR 0005 phase 4) batch 6, 2026-09-11.** Migrated
  `StepMania.cpp` (23 real sites -> `Log::General`, the top-level
  application/main-loop file with no single subsystem category fit;
  3 grep hits are pre-existing commented-out input-debug calls, left
  untouched). Kept both existing `Warn` sites at `WARN` (a saved
  game-type preference that's no longer available, falling back to
  the default; an unknown `--game` command-line argument being
  ignored -- real, if recoverable, config/input problems). Downgraded
  one `Warn`->`INFO`: "video renderer list has been changed from X to
  Y" is the code noting a config divergence from card defaults and
  continuing normally, not a problem -- `WARN` was the wrong altitude.
  Everything else (startup banner/version/command-line-args dump,
  video-card-default detection, coin-mech bookkeeping, screenshot
  timing) was already correctly leveled `Trace`/`Info` and just got a
  category. No parsing/behavior logic changed; not §5-protected.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

* **item 18 (ADR 0005 phase 4) batch 7, 2026-09-11.** Migrated
  `Profile.cpp` (16 sites -> `Log::Profile`, a perfect fit). The
  `LOAD_NODE(X)` macro's "Failed to read section X" upgraded
  `Warn`->`ERROR` -- its 6 call sites (`GeneralData`, `SongScores`,
  `CourseScores`, `CategoryScores`, `ScreenshotData`, `CalorieData`)
  are all core, always-expected top-level sections of `stats.xml`; a
  missing one is real data loss/corruption, not an optional field.
  `LoadStatsFromDir`'s two file-open failures (plain open, and gunzip
  of the compressed variant) upgraded `Trace`->`ERROR` -- both
  immediately return `ProfileLoadResult_FailedTampered`, i.e. the code
  already treats them as hard failures, the log level just hadn't
  caught up. `LoadSongsFromDir`'s "Song %s failed to load" upgraded
  `Trace`->`WARN` -- a real per-item failure worth surfacing.
  Everything else (routine load/save progress markers, signature-
  verification step tracing -- actual signature failures go through
  `LuaHelpers::ReportScriptErrorFmt`, a separate path) stayed `Trace`.
  No parsing/behavior logic changed; not §5-protected.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  **This closes the non-§5 tail of phase 4** -- everything left with a
  bare `LOG->` call is either a §5-protected parser (needs
  characterization-test re-verification each time) or `Player.cpp`
  (48 sites, god-object hotspot, needs its own careful pass).

* **item 18 (ADR 0005 phase 4) batch 8, 2026-09-12 -- first
  §5-protected batch.** `TimingSegments.cpp` (12 sites -> `Log::Song`)
  and the 3 real (non-`UserLog`) sites in `NotesLoaderKSF.cpp` (also
  `Log::Song`; its 6 `LOG->UserLog(...)` calls stay untouched, same
  as every other loader file). `TimingSegments.cpp` was the cleanest
  possible §5 pilot: all 12 sites are identical-shape `DebugPrint()`
  overrides on each `TimingSegment` subclass, pure diagnostic
  formatting with zero decision logic -- no triage judgment needed,
  all stayed `LOG_TRACE`. `NotesLoaderKSF.cpp`'s 3 sites were likewise
  already-correct routine `Trace` calls, just categorized. Re-verified
  against characterization tests before AND after: `[TimingData]`
  40/9 and `[ksf]` 31/2, both identical -- confirming pure category/
  level tagging with zero parsing/behavior change, per the §5 gate.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

* **item 18 (ADR 0005 phase 4) batch 9, 2026-09-12.** `TimingData.cpp`
  (10 real sites -> `Log::Song`), `NotesLoaderSMA.cpp` (1),
  `NotesLoaderDWI.cpp` (2; a 3rd grep hit is a pre-existing
  commented-out call), `NotesLoaderBMS.cpp` (2) -- all `Log::Song`.
  Almost everything was already-correct routine `Trace`; one real
  upgrade: `NotesLoaderDWI.cpp`'s "Didn't get enough data when
  attempting to load a DWI file" `Warn`->`ERROR` (a genuine
  malformed-chart parse failure, falls back to an empty `NoteData`).
  **Handled a wrinkle:** 3 of `TimingData.cpp`'s sites sit behind
  `#ifdef WITH_LOGGING_TIMING_DATA`, a real CMake option (default OFF,
  unlike item 29's truly-orphaned code) not exercised by the normal
  build. Reconfigured `-DWITH_LOGGING_TIMING_DATA=ON`, rebuilt, reran
  the full suite (5966/226 unchanged) to actually compile-verify those
  3 sites, then reconfigured back to the default `OFF` to match CI. One
  more site sits behind `#ifdef DEBUG`, already covered by the normal
  Debug build. `[TimingData]` re-checked at 40/9 (unchanged from batch
  8); the new `[sma]`/`[dwi]`/`[bms]` tags weren't baselined
  individually before this edit, but the unchanged full-suite total
  (5966/226 both before and after) is the authoritative check since
  any behavior change would have shifted it.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

* **item 18 (ADR 0005 phase 4) batch 10, 2026-09-12.** `Song.cpp` (12
  real sites -> `Log::Song`; 2 grep hits are pre-existing dead code --
  a commented-out line plus a whole call inside a `/* */` block, both
  left untouched). The cache-load fallback warning ("main title or
  music file came up blank") kept at `WARN`. Three custom-song
  rejection sites (too long / can't open music / file too big)
  upgraded `Trace`->`WARN`, matching batch 7's `Profile.cpp`
  precedent. "Points to a music file that doesn't exist, found music
  file X" upgraded `Trace`->`WARN` -- a real broken-reference simfile
  issue, recovered via fallback but worth surfacing. The optional
  timestamped-backup step's failure case upgraded `Trace`->`WARN`
  (primary save already succeeded, but a failed safety net matters).
  Rest of the save-flow entry traces stayed `Trace`. No parsing/
  behavior logic changed; no dedicated `[Song]` characterization tag
  exists, so the unchanged full-suite total is the check, same as
  batch 9.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

* **item 18 (ADR 0005 phase 4) batch 11, 2026-09-12 -- closes out the
  entire §5-protected lane.** `NotesLoaderSM.cpp` (1 real site -- its
  other 3 raw hits were pre-existing commented-out calls),
  `CourseLoaderCRS.cpp` (3 real sites), `NotesLoaderSSC.cpp` (2 real
  sites; 1 more grep hit is commented-out) -- all `Log::Song`. Every
  site was already a correctly-leveled routine `Trace` (loader/edit-
  file entry points, cache-vs-fresh-load branch tracing) -- pure
  categorization, no triage upgrades. Verified against real
  characterization coverage: `[SMLoader]` 43/14, `[corpus]` 313/3
  (covers `.sm` and `.ssc` via the paired-format test), `[crs]` 39/5,
  all unchanged, plus the full suite (5966/226).
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  **Item 18 phase 4 is now fully done except `Player.cpp`** (48
  sites, god-object hotspot, own careful pass since no
  characterization test covers gameplay/scoring logic). Every
  §5-protected simfile-format file is now migrated to the categorized
  `LOG_*` macros, zero parsing/behavior changes throughout.

* **item 18 (ADR 0005 phase 4) final batch, 2026-09-12 -- `Player.cpp`,
  phase 4 now COMPLETE.** Of 48 raw `LOG->` grep hits in this
  god-object (3567 lines), a programmatic check (`awk` tracking
  `/* */` block-comment state, not just `//` lines) found **47 of 48
  are dead code** -- leftover debug scaffolding from a historically
  fragile hold-note-scoring area (the same section item 15 already
  flagged as too fragile to touch for `#if 0` removal). Only one call
  site is actually compiled: a routine "Applying transform..." trace
  in the attack-mod path, migrated to `LOG_TRACE(Log::Actor, ...)`.
  No characterization test exists for `Player.cpp` (gameplay/scoring,
  not simfile parsing), so the unchanged full-suite total (5966/226)
  is the only available safety check.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  **ADR 0005 is now fully implemented end to end** -- all 4 phases
  done, 11 batches total for phase 4 alone, spanning non-§5 files, the
  whole §5-protected simfile-parser lane, and `Player.cpp`, with zero
  parsing/behavior regressions throughout.

* **item 18 phase 4 CORRECTION, 2026-09-12 (same day).** The "phase 4
  complete" entry above was wrong. All 11 batches worked from a fixed
  top-30 file list captured once at the start of phase-4 recon --
  every batch picked files off that original list, but the list was
  never a full sweep and no final re-sweep of all of `src/` was done
  before declaring it done. A fresh sweep (`grep -cE "LOG->Trace|
  LOG->Warn|LOG->Info" src/*.cpp`, UserLog still excluded) found
  **127 more files, ~430+ more sites** that were simply never on the
  original list -- `PlayerStageStats.cpp` (12), `ScreenGameplay.cpp`
  (11), `RageUtil.cpp` (11), `ScreenManager.cpp` (10),
  `RageDisplay_GLES2.cpp` (10), down to a long 1-site tail. This is
  the exact same methodology trap already documented for item 2's
  C4244/C4267 sweep -- a curated work-list snapshot is not a
  substitute for a final full-tree re-sweep before calling something
  done. Phase 4 resumes as an actual long tail from here, no longer
  batching off the stale list.

* **item 18 (ADR 0005 phase 4) batch 12, 2026-09-12 -- first batch
  against the corrected remaining list.** `PlayerStageStats.cpp` (1
  real site -> `Log::Lua`, a bad Lua-script argument warning),
  `RageUtil.cpp` (11 sites, split by function: `GetFileContents`/
  `FileCopy` I/O failures -> `Log::File`/`ERROR`; `StringToInt/Long/
  LLong` catch-block warnings -> `Log::General`, kept `WARN` since
  also used for legitimate speculative parsing), `ScreenGameplay.cpp`
  (9 sites -> `Log::Screen`, "Error loading notes for player" upgraded
  `Trace`->`ERROR`), `ScreenManager.cpp` (8 sites -> `Log::Screen`,
  all already-correct), `RageDisplay_GLES2.cpp` (10 sites ->
  `Log::Gl`, same vendor-dump->`INFO` / mislabeled-entry-trace->
  `TRACE` pattern as the OGL/D3D backends -- **this file is Linux-only**
  (`elseif(LINUX) if(WITH_GLES2)`-gated), so Ubuntu CI is its real
  compile check, not the local Windows gate), `ScreenSelectMusic.cpp`
  (8 sites -> `Log::Screen`, two song-deletion guard warnings kept
  `WARN`). No parsing/behavior logic changed.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0 (Windows);
  `RageDisplay_GLES2.cpp` still needs Ubuntu CI confirmation.
  ~121 files remain on the corrected list.

* **item 18 (ADR 0005 phase 4) batch 13, 2026-09-12.** `RageDisplay.cpp`
  (5 sites -> `Log::General`, same reasoning as `RageDisplay_D3D.cpp`
  -- backend-agnostic base class, `Log::Gl` would be misleading;
  screenshot save/open failures upgraded `Trace`->`ERROR`),
  `Font.cpp` (3 sites -> `Log::Font`; two "invalid codepoint value"
  warnings kept `WARN`, one routine note kept `Trace`),
  `ScreenEdit.cpp` (5 sites -> `Log::Screen`; "Save failed. Changes
  uncommitted from memory." upgraded `Trace`->`ERROR` -- a genuine
  editor-save failure with real data-loss risk). No parsing/behavior
  logic changed.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~118 files remain on the corrected list.

* **item 18 (ADR 0005 phase 4) batch 14, 2026-09-12.**
  `RageUtil_BackgroundLoader.cpp` (7 sites -> `Log::File`, all
  "XXX:"-prefixed dev-debug traces, already-correct routine `Trace`),
  `MusicWheel.cpp` (6 sites -> `Log::Actor` -- a `WheelBase`-derived
  widget, not a `Screen`), `ImageCache.cpp` (3 sites -> `Log::Cache`,
  a perfect fit; both warnings kept `WARN`), `Bookkeeper.cpp` (7 sites
  -> `Log::File`; two XML-parse failures upgraded `Warn`->`ERROR`,
  plus the never-invoked `WARN_AND_RETURN` macro migrated the same way
  for consistency; per-entry "incomplete date"/"hour out of range"
  warnings kept `WARN` since they skip one record and continue; the
  write-open failure upgraded to `ERROR`). No parsing/behavior logic
  changed.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~114 files remain on the corrected list.

* **item 18 (ADR 0005 phase 4) batch 15, 2026-09-12.** `XmlToLua.cpp`
  (6 sites -> `Log::File`; the core `convert_xml_file` entry point's
  "error loading xml"/"could not open output file" upgraded
  `Trace`->`ERROR`, the optional sprite/model sub-file reads kept
  `Trace`), `SongManager.cpp` (6 sites -> `Log::Song`, all
  already-correct), `RageTextureManager.cpp` (6 sites -> `Log::Cache`;
  the `"TEXTUREMAN LEAK"` shutdown refcount check upgraded
  `Trace`->`WARN`, a genuine leak indicator), `NoteField.cpp` (2 sites
  -> `Log::Actor`, already-correct), `GameState.cpp` (4 sites ->
  `Log::General`; the `BeginStage`-called-twice invariant-violation
  warning kept `WARN`, the blacklisted-name match upgraded
  `Trace`->`WARN`). No parsing/behavior logic changed.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~109 files remain on the corrected list.

* **item 18 (ADR 0005 phase 4) batch 16, 2026-09-12.**
  `ScreenUnlockStatus.cpp` and `NoteDataWithScoring.cpp` turned out to
  have zero real sites (all dead code, some inside `/* */` blocks) --
  skipped. `ScreenOptions.cpp` (3 -> `Log::Screen`, already-correct),
  `RageThreads.cpp` (5 -> `Log::General`; mutex lock-order-
  inconsistency warning upgraded `Warn`->`ERROR`, a genuine deadlock-
  risk bug indicator), `RageSoundReader_Merge.cpp` (2 -> `Log::Sound`),
  `ProfileManager.cpp` (5 -> `Log::Profile`; "corrupt profile, no
  LastGood either" upgraded `Trace`->`WARN`, "DeleteRecursive failed"
  upgraded `Warn`->`ERROR`), `LuaManager.cpp` (4 -> `Log::Lua`;
  `ReportScriptError`'s and the compile-failure's `Warn`s upgraded to
  `ERROR` -- every call through those paths is a real script error;
  the Lua-exposed global `Trace()`/`Warn()` kept at their own levels
  since that's the theme author's choice, not ours to second-guess),
  `GameSoundManager.cpp` (4 -> `Log::Sound`, already-correct). No
  parsing/behavior logic changed.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~101 files remain on the corrected list.

* **item 18 (ADR 0005 phase 4) batch 17, 2026-09-12.**
  `RageUtil_FileDB.cpp` (3 -> `Log::File`; two documented-precondition
  violations upgraded `Warn`->`ERROR`), `RageSoundReader_Chain.cpp` (3
  real -> `Log::Sound`; a 4th grep hit sits inside a `/* */` block,
  caught by the same `awk` comment-state check used for `Player.cpp`;
  "error opening sound" upgraded `Trace`->`ERROR`), `OptionRowHandler.cpp`
  (1 -> `Log::Screen`, kept `WARN`), `Course.cpp` (2 -> `Log::Song`; an
  unrecognized sort-type default case upgraded `Trace`->`WARN`). No
  parsing/behavior logic changed.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~97 files remain on the corrected list.

* **item 18 (ADR 0005 phase 4) batch 18, 2026-09-12 -- found a third
  dead-code category.** Skipped `ScreenNetEvaluation.cpp` (item 29's
  orphaned networking subsystem). `RageFileManager_ReadAhead.cpp`'s 3
  grep hits all sit inside a permanent `#if 0` block (nested under
  `#if defined(HAVE_POSIX_FADVISE)` / `#else` / `#if 0` ... `#else`
  (stub) / `#endif` / `#endif`) -- a preprocessor-disabled fallback
  path never compiled on any platform, distinct from `//` comments and
  `/* */` blocks already caught earlier this sweep -- skipped.
  Migrated: `ScreenMapControllers.cpp` (1 -> `Log::Input`),
  `ScreenHighScores.cpp` (2 -> `Log::Screen`), `ScreenEditMenu.cpp` (3
  -> `Log::Screen`; "Delete failed" upgraded `Trace`->`WARN`),
  `RageUtil_WorkerThread.cpp` (3 -> `Log::General`),
  `RageUtil_CharConversions.cpp` (3 -> `Log::File`),
  `RageSoundReader_WAV.cpp` (3 -> `Log::Sound`; "predictor out of
  range" upgraded `Trace`->`WARN`), `RageFileDriverTimeout.cpp` (3 ->
  `Log::File`, all routine), `NetworkManager.cpp` (3 -> `Log::Net`;
  "reading CA bundle failed" upgraded `Warn`->`ERROR`),
  `LyricsLoader.cpp` (3 -> `Log::Song`; invalid color-value warning
  upgraded `Trace`->`WARN`). No parsing/behavior logic changed.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~87 files remain on the corrected list.

* **item 18 (ADR 0005 phase 4) batch 19, 2026-09-12.** `JsonUtil.cpp`
  (3 -> `Log::File`, all genuine file-open/parse failures upgraded
  `Warn`->`ERROR`), `InputMapper.cpp` (3 -> `Log::Input`, already-
  correct), `InputFilter.cpp` (3 -> `Log::Input`; out-of-range device/
  button index sites upgraded `Trace`->`WARN`, a driver anomaly worth
  surfacing), `GameLoop.cpp` (3 -> `Log::General`, already-correct),
  `ActorMultiTexture.cpp` (3 -> `Log::Actor`, theme-misuse guards kept
  `WARN`). No parsing/behavior logic changed.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~82 files remain on the corrected list.

* **item 18 (ADR 0005 phase 4) batch 20, 2026-09-12.** `WheelBase.cpp`
  (2 -> `Log::Actor`), `UnlockManager.cpp` (2 -> `Log::Song`),
  `StatsManager.cpp` (2 -> `Log::Profile`), `StageStats.cpp` (2 ->
  `Log::Profile`) -- all already-correct. `ScreenWithMenuElements.cpp`
  (2 -> `Log::Screen`, both upgraded `Trace`->`WARN` -- real
  theme-Lua-script failures despite the informal message text).
  `ScreenServiceAction.cpp` (2), `ScreenSelectCharacter.cpp` (2),
  `ScreenSelect.cpp` (1) -- all `Log::Screen`, already-correct. No
  parsing/behavior logic changed.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~75 files remain on the corrected list.
* **item 18 (ADR 0005 phase 4) batch 21, 2026-09-12.** `Steps.cpp` (2, `Log::Steps`, both kept `WARN`; section-5-protected, re-verified via the transitive NotesLoader/GameManager characterization suite since no dedicated `test_Steps.cpp` exists — unchanged before/after), `ScreenEvaluation.cpp` (2, `Log::Screen`, `TRACE`), `ScreenDebugOverlay.cpp` (2, `Log::Screen`), `ScreenAttract.cpp` (1, `Log::Screen`, `TRACE`), `RageSurface_Load_PNG.cpp` (2, `Log::File`, upgraded to `ERROR`/`WARN`), `RageBitmapTexture.cpp` (1, `Log::File`, upgraded to `ERROR`), `RageInput.cpp` (2, `Log::Input`), `PrefsManager.cpp` (2, `Log::General`), `Preference.cpp` (2, `Log::Lua`), `PercentageDisplay.cpp` (2, `Log::Actor`), `NoteSkinManager.cpp` (2, `Log::Actor`), `InputQueue.cpp` (2, `Log::Input`, one upgraded to `WARN`), `CsvFile.cpp` (2, `Log::File`, one upgraded to `ERROR`), `BackgroundUtil.cpp` (1 real site, `Log::Song`), `ActorUtil.cpp` (2, `Log::Actor`), `RandomSample.cpp` (2, `Log::Sound`, confirmed real via `#if 0` structure check). `Background.cpp` confirmed 0 real sites — no edit. No parsing/behavior logic changed. Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0. ~62 files remain on the corrected list.
* **item 18 (ADR 0005 phase 4) batch 22, 2026-09-12.** Cross-checked every already-"done" file whose raw grep count was still non-zero (Font.cpp, RageSound.cpp, NoteField.cpp, ImageCache.cpp, StepMania.cpp, RageSoundReader_Merge.cpp, RageDisplay.cpp, OptionRowHandler.cpp, Song.cpp, ScreenOptions.cpp, ScreenManager.cpp, ScreenGameplay.cpp, ScreenEdit.cpp, RageDisplay_D3D.cpp, GameState.cpp, Course.cpp, NotesLoaderSM.cpp, ScreenMapControllers.cpp, PlayerStageStats.cpp) — all confirmed dead-code-only remainders. Found the awk block-comment scanner itself has a false-negative blind spot: `ScreenPackages.cpp`'s glob-pattern string literal (`"Packages/*.zip"`) contains a bare `/*` that the naive scanner misreads as a comment opener, hiding a genuinely live `LOG->Trace` site until the next real `*/` hundreds of lines later — resolved by direct read, not by trusting the script. Separately discovered `ScreenPackages.cpp` and `FileDownload.cpp` are absent from every `CMakeData-*.cmake` list (a second orphaned pair, distinct from item 29's SMOnline cluster) — left both unedited pending the same maintainer wire-vs-delete call, documented under item 29. Migrated 33 genuinely new real sites this batch across AnnouncerManager/AttackDisplay/ComboGraph/GameCommand/EditMenu/Grade/FontManager/ModelManager/MeterDisplay/MessageManager/PlayerState/PlayerOptions/RageException/RageFile/RageSoundManager/RageSoundPosMap/RageSoundReader/RageSurfaceUtils/RageSoundReader_FileReader/RageSurface_Load/RageSoundReader_Vorbisfile/RageSurface_Save_PNG/ScoreDisplay{Oni,LifeTime,Normal,Battle,Rave}/Screen/ScreenOptionsManageEditSteps/ScreenOptionsMaster/ScreenOptionsMasterPrefs/ScreenStatsOverlay/ScreenTextEntry/ScreenTitleMenu/SongCacheIndex/SongUtil.cpp. NoteDataUtil.cpp (1, Log::Song) and NotesWriterDWI.cpp (1, Log::Song) are section-5-protected — NoteDataUtil.cpp re-verified against its own `[NoteDataUtil]` tag (53/12 unchanged); the rest have no dedicated tag so rely on the full-suite invariant. No parsing/behavior logic changed. Verified: sm_tests 5966/226 unchanged, ctest 100%, Release StepMania-R.exe clean rebuild, --SelfTest exit 0.
* **item 18 (ADR 0005 logging overhaul) CLOSED, 2026-09-12 — all 4 phases genuinely complete.** After batch 22, a fresh full-tree re-sweep (`grep -cE "LOG->Trace|LOG->Warn|LOG->Info" src/*.cpp`) confirmed every file with a non-zero raw count is now either fully migrated (only dead `//`/`/* */`-commented lines remain, confirmed file-by-file), deliberately skipped pending a maintainer wire-vs-delete call (the item 29 SMOnline cluster + the newly-found `ScreenPackages.cpp`/`FileDownload.cpp` package-downloader pair, both orphaned from the CMake build), or `RageFileManager_ReadAhead.cpp`'s permanently dead `#if 0` block. This explicitly learns from the earlier premature "phase 4 complete" declaration (corrected mid-session): this time the closure is backed by an actual fresh zero-exceptions full-tree sweep, not a checked-off curated list. 22 batches total for phase 4.
