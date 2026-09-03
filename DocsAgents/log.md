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
