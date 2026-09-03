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
