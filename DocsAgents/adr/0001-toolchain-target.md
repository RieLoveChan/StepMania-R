---
type: Architecture Decision
title: Toolchain and modernization target
description: What "modern" means for this fork — C++ standard, CMake floor, platform priority, and what gets dropped.
tags: [adr, toolchain, cpp, cmake, platforms]
---

# Status

**Accepted** — 2026-09-02. All open questions resolved (see *Decision*).
The renderer-backend question (former open questions B and C) is
**deferred to ADR 0004 (renderer strategy)**, not left open here.

# Context

StepMania-R exists to modernize a long-inactive codebase and ship
releases ([`../index.md`](../index.md)). "Modernize" needs a concrete
target or it has no definition of done. Current state as of this ADR:

- `CMakeLists.txt`: `cmake_minimum_required(VERSION 3.20)`.
- `src/CMakeLists.txt:88`: `CXX_STANDARD 17`, `CXX_STANDARD_REQUIRED ON`.
- Warnings: `-Wall -Wextra` (GCC/Clang) / `/W4` (MSVC) with blanket
  suppressions (`src/CMakeLists.txt:126-132`); no `-Werror` except
  `type-limits`. No measured count.
- Submodules for `extern/` libs; GitHub Actions CI on Win/macOS×2/Linux.
- Legacy carried forward: `RString` (`typedef StdString::CStdString`,
  `global.h:107`) in 723 files / ~8.4k uses; `RageThreads` predating
  `std::thread`; prebuilt FFmpeg Windows binaries in `extern/ffmpeg-w32/`.

So: the **build system** is already fairly modern; the **source** is not.

# Decision

## Settled

1. **Platform priority: Windows > macOS > Linux** (`AGENTS.md` §3).
   Windows is the reference platform; MSVC is the reference compiler.
   macOS/Linux are kept building via CI but not actively developed.
2. **CMake floor stays at 3.20.** Recent enough; no reason to move.
3. **C++ standard stays at C++17 for now.** Do not stack a `-std` bump on
   an unmeasured, unstable base. Revisit C++20 (→ a new ADR) only after
   the warning baseline ([`../baseline.md`](../baseline.md)) is captured
   and trending down.
4. **Modernization is incremental and per-subsystem by default**, behind
   the §4 verification gate. Rationale is *regression safety on a
   codebase with no test net* and limited review capacity — not upstream
   (ADR [0002](./0002-independent-project.md)). Once "step 2" (test
   harness + headless smoke test) lands, bigger single changes become
   viable. Mechanical passes use
   [`../playbooks/clang-tidy-subsystem-pass.md`](../playbooks/clang-tidy-subsystem-pass.md).
5. **`RString` → `std::string` is a declared goal.** Per-subsystem is the
   *preferred* execution (reviewable, safe) but a repo-wide migration is
   now permitted (ADR 0002) once there is a test net to catch
   regressions. See
   [`../playbooks/migrate-rstring.md`](../playbooks/migrate-rstring.md).
6. **First cleanup cut (low-risk):** delete `.travis.yml` (dead CI);
   remove the dead `src/irc/` freenode notifier (`CMakeProject-irc.cmake`
   + refs in `src/CMakeLists.txt`). `README.md` was already replaced with
   a project statement, so no badges to fix. The `src/irc/` removal
   touches CMake → verify with a build, through the §4 gate.

7. **Warnings / `-Werror` policy (was open question A).** Adopt a
   **curated `-Werror` set that grows monotonically**. A warning category
   is promoted to `-Werror=<cat>` (MSVC `/we<NNNN>`) only once it is at
   **zero across `src/`** (externals stay excluded, as today). Enforcement
   is CI-only: a `WITH_WERROR` CMake option, **ON in CI, OFF by default**
   for local/contributor builds, so a compiler-version bump that adds new
   diagnostics cannot break everyone's build. Every promotion is recorded
   in [`../baseline.md`](../baseline.md) with the PR that cleaned it.

8. **FFmpeg on Windows (was D).** **Direction: drop the committed
   `extern/ffmpeg-w32/` blob and get FFmpeg from the `extern/ffmpeg`
   submodule**, so this project controls the FFmpeg version instead of
   inheriting a frozen ~5.x prebuilt. The submodule already exists in
   `.gitmodules` (currently uninitialized) and `CMake/SetupFfmpeg.cmake`
   already builds it for macOS/Linux via ExternalProject
   (`configure` + `make`).
   - **Sequencing:** not first. Do it as an early bounded change *after*
     the green baseline build + smoke test exist, so a regression is
     catchable.
   - **Until then:** keep `extern/ffmpeg-w32/` (load-bearing on Windows,
     `StepmaniaCore.cmake:179+`, `:201-209`); record its exact versions
     (avcodec-59 / avformat-59 / avutil-57 / swscale-6) and origin in
     `../baseline.md`.
   - **Two viable Windows paths, pick during execution:** (a) make
     `SetupFfmpeg.cmake` build the submodule on Windows via the maintainer's
     msys2 (`C:\msys64`) + nasm — works, but every contributor then needs
     msys2; (b) build the DLLs once per FFmpeg bump in CI and consume that
     artifact — more robust, no local msys2 dependency. Lean (b).
   - Tracked as backlog item 7.
   - **Executed (2026-09-04), path (b).** `extern/ffmpeg-w32/` is gone;
     `CMake/SetupFfmpegWin32.cmake` downloads the CI-built artifact. See
     backlog item 7 (Closed) and `baseline.md` for details.

9. **Windows / MSVC floor (was E). → SUPERSEDED by ADR
   [0003](./0003-platform-support-floors.md).** Originally set a Windows
   10 x64 floor; ADR 0003 raises it to **Windows 11 x64** and adds the
   macOS / Linux floors. Reference toolchain stays **MSVC v143 / Visual
   Studio 2022** (or newer).

10. **C++20 (was F): after the baseline.** Stay C++17 until
    `../baseline.md` warning/tidy counts are captured and trending down;
    then evaluate C++20 in its own ADR. A `-std` bump on an unmeasured
    base only adds noise.

## Deferred to ADR 0004 (renderer strategy)

- **Drop the D3D9 renderer (`RageDisplay_D3D`)?** (was B)
- **Drop `WITH_GLES2`?** (was C)

Both depend on the overall renderer direction — clean the legacy GL
backend (`RageDisplay_Legacy`, still fixed-function) / modernize to GL
3.3 core / adopt an abstraction layer (bgfx et al.) — which a toolchain
ADR should not pre-empt. **Until ADR 0004:** `RageDisplay_D3D` and the
GLES2 path are **frozen** — kept compiling, no investment, no new
features. GLES2 is the leading removal candidate there (Linux-only, P3,
~26 TODO/HACK markers, no dependent user base).

# Consequences

- "Modern" now has a concrete, accepted definition, bounded by the §4
  gate (safety) rather than by upstream compatibility (ADR 0002).
- The warning/tidy baseline is the backlog and the progress metric
  ([`../modernization-backlog.md`](../modernization-backlog.md),
  [`../baseline.md`](../baseline.md)); §7 gives it an enforcement path.
- C++20 and the D3D9/GLES2 drops are intentionally postponed — the price
  of not destabilizing an untested base. Each gets its own ADR when its
  precondition is met (baseline captured / renderer direction chosen).
- Immediate actionable items from this ADR: the §6 cleanup cut; add
  `WITH_WERROR` (default OFF, ON in CI); document the FFmpeg binary
  provenance in `baseline.md`; state the Win11/v143 floor in the build
  docs.
