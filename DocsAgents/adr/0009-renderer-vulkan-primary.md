---
type: Architecture Decision
title: Renderer strategy II - Vulkan primary, OpenGL fallback
description: Make Vulkan the primary renderer backend and keep the existing OpenGL backend as a maintained fallback; supersedes ADR 0004's "migrate to GL 3.3 core" direction while keeping its D3D9/GLES2 removals.
tags: [adr, renderer, vulkan, opengl, proposed]
---

# Status

**Accepted** - 2026-10-04. Maintainer decision: Vulkan as the primary
renderer, OpenGL kept as a maintained fallback, D3D9 removed. The
maintainer also considered keeping D3D9 and adding Direct3D 11/12 and
chose this option instead (see Alternatives).

**Stage 0 implemented and closed - 2026-10-04.** GLES2 removed in
`52802ec7b8`, D3D9 removed in `410aadbde0`; CI green on all 7 jobs for
each. Stages 1-4 (Vulkan prototype, full backend, parity gate and default
flip, fallback review) are not started.

**Supersedes ADR 0004 in part.** Only 0004's first decision ("migrate to
GL 3.3 core profile") is replaced. 0004's other two decisions - **drop the
D3D9 renderer** and **drop `WITH_GLES2`** - stay in force and are carried
out here as stage 0. ADR 0004's status line and
[`index.md`](./index.md) carry a "partially superseded by 0009" note; the
body of 0004 is not edited (ADRs are immutable once `Accepted`).

# Decision

- **Primary renderer: Vulkan.** A new `RageDisplay_Vulkan` backend, written
  against the existing `RageDisplay` interface, becomes the default
  renderer once it passes the parity gate below. Until then it is opt-in
  (`VideoRenderers=vulkan`) and the defaults do not change.
- **OpenGL stays as a fallback.** The existing OpenGL backend
  (`RageDisplay_Legacy`, `src/RageDisplay_OGL.*`) is kept, built, and
  tested in CI. It is selected by `VideoRenderers` like today and is the
  automatic fallback when Vulkan initialisation fails (no Vulkan driver,
  unsupported GPU, MoltenVK limitation). The default order becomes
  `"vulkan,opengl"` on every platform when Vulkan reaches the parity gate.
- **The fallback is maintained, not frozen, and not extended.** The OpenGL
  backend keeps building and passing tests and gets bug fixes. New
  `RageDisplay` capabilities do not have to land there first; a feature
  the fallback cannot provide is capability-gated, not blocked.
- **No GL 3.3 core rewrite of the fallback.** ADR 0004's rewrite of
  `RageDisplay_OGL` to a core-profile shader pipeline is **not**
  undertaken. The OpenGL backend stays on its current code path. The one
  piece of that plan worth keeping is replacing fixed-function state with
  shaders, and that work is done once, inside the Vulkan backend.
- **macOS uses MoltenVK** (Vulkan translated to Metal) for the Vulkan
  backend. A native Metal backend is **not** chosen now (see Alternatives).
- **Drop D3D9 and `WITH_GLES2`** exactly as ADR 0004 decided. Windows keeps
  OpenGL as its fallback after D3D9 goes; Vulkan then becomes the first
  choice on Windows when it is the default.
- **`RageDisplay_Null` is unchanged.** `--SelfTest`, `sm_tests` and
  headless runs keep using it; no Vulkan or GPU is needed to run the test
  suite.

# Context

- **Backends today.** The engine ships four: `RageDisplay_Legacy`
  (OpenGL compatibility profile, `RageDisplay_OGL.cpp` 2665 lines),
  `RageDisplay_D3D` (Direct3D 9, Windows only, 1448 lines),
  `RageDisplay_GLES2` (Linux, `WITH_GLES2`, 881 lines) and
  `RageDisplay_Null` (153 lines). They implement the abstract
  `RageDisplay` class (`src/RageDisplay.h`, 44 pure virtuals).
- **Selection.** `CreateDisplay()` (`src/StepMania.cpp`) splits the
  `VideoRenderers` preference on commas and tries each name in order
  (`opengl`, `d3d`, `null`, plus `gles2` when built). First-run defaults
  are `"opengl"` on generic macOS/Linux and `"opengl,d3d"` on generic
  Windows. Adding a backend or changing the order is therefore a
  data/wiring change, not an engine change.
- **The interface is shaped like fixed-function.** `RageDisplay` exposes
  `SetLighting`/`SetLightOff`, `SetSphereEnvironmentMapping`,
  `SetCelShaded`, `SetAlphaTest`, `SetTextureMode` (per-unit combiner
  modes), `SupportsPerVertexMatrixScale` and immediate-style
  `Draw*Internal(const RageSpriteVertex[], n)` entry points, plus
  `RageCompiledGeometry` for static meshes. A Vulkan backend has no
  fixed-function pipeline, so each of these has to be expressed as shader
  and pipeline state. `RageDisplay_OGL.cpp` still has about 38 call sites
  of the fixed-function matrix/vertex-array API.
- **Existing shader use is limited but real.** `Data/Shaders/GLSL/` holds
  GLSL for the advanced blend modes (color burn, color dodge, hard mix,
  overlay, screen) and for effects (cel, shell, distance field).
  Everything else is fixed-function. A Vulkan backend must provide
  equivalents of all of these, and they are the natural seed for the
  shader set built in stage 2.
- **Why not stay on OpenGL.** Khronos stopped developing OpenGL at 4.6
  (2017). Apple froze it at 4.1 and deprecated it. For a project with a
  10+ year horizon (ADR 0002) a frozen API is a liability; Vulkan is
  actively developed and is the common modern denominator on Windows and
  Linux.
- **Platform floors** (ADR 0003): Windows 11 x64 (P1), latest two macOS on
  arm64 (P2), current mainstream Linux (P3). Vulkan drivers ship with GPU
  vendor drivers on Windows and Linux; macOS needs MoltenVK.
- **Test surface.** There are no renderer-specific tests in `tests/`
  today; the Null backend is what the suite exercises. `RageDisplay` has
  `CreateScreenshot()`, which makes screenshot comparison between two
  backends possible.

# Alternatives considered

- **OpenGL 3.3 core (ADR 0004's direction).** Lowest cost and one
  portable code path, but it is the same frozen API: it only postpones the
  move off OpenGL and the rewrite cost would be spent twice. Rejected as
  the destination.
- **Native Metal on macOS plus Vulkan elsewhere.** Gives native macOS
  rendering but adds a third backend for a single platform. Deferred;
  revisit only if MoltenVK proves inadequate (see Open questions).
- **Direct3D 11/12 on Windows.** Good Windows drivers, but it is a
  Windows-only backend beside the portable one. Rejected: Vulkan already
  covers Windows. Note that D3D9 cannot be "upgraded" to D3D11/12: that is
  a new backend written from scratch (no fixed function, shaders required),
  not a port, so keeping D3D9 and adding D3D11/12 would mean more backends,
  each needing manual GPU verification because CI has no GPU. If Vulkan
  turns out to have real driver problems on Windows, a Direct3D 11 backend
  can be reconsidered by a new ADR.
- **Keep D3D9 as it is and add Vulkan.** Costs nothing in D3D9 itself
  (about 1448 lines; it builds against the current Windows SDK with no
  legacy D3DX/DxErr dependency, and Windows 11 still ships D3D9) and removes
  nothing for users. Not chosen: it leaves four backends to maintain and
  test by hand for a renderer that is already only the second choice on
  Windows (`"opengl,d3d"`).
- **A rendering abstraction library (bgfx, SDL_GPU, wgpu/WebGPU).** One
  backend that targets Vulkan, D3D and Metal underneath. It trades our own
  Vulkan backend for a large dependency and a rewrite of `RageDisplay`
  on top of it. Not chosen now; the `RageDisplay` interface already is the
  abstraction.

# Plan (staged; each stage is its own commits with CI between them)

- **Stage 0 - remove D3D9 and GLES2** (already decided by ADR 0004).
  `RageDisplay_D3D.*`, its `CMakeData-rage.cmake` and `StepMania.cpp`
  wiring, and the `WITH_GLES2` branch of `RageDisplay_GLES2.cpp`.
  DirectInput and DirectSound (`FindDirectX.cmake`, `DirectXHelpers.cpp`)
  are separate users of DirectX and stay. Windows fallback after this is
  OpenGL only; note the behaviour change in `log.md`.
- **Stage 1 - prototype behind `VideoRenderers=vulkan`.** Instance,
  device, per-platform surface (each `LowLevelWindow` implementation
  provides a `VkSurfaceKHR`), swapchain, clear, one textured quad, mode
  change, `CreateScreenshot`. Default order unchanged. The prototype
  decides the open questions below before more is built.
- **Stage 2 - full `RageDisplay` implementation.** Textures and formats,
  blend modes, depth state, `RageCompiledGeometry`, the `Draw*Internal`
  paths, and shaders that reproduce the fixed-function features the
  interface exposes. Shaders are written once (GLSL compiled to SPIR-V at
  build time) and cover all pipeline permutations.
- **Stage 3 - parity gate, then flip the default** to `"vulkan,opengl"`.
  The gate is: (a) every screen of the default theme renders the same as
  the OpenGL backend by screenshot comparison within a stated tolerance;
  (b) vsync, windowed/fullscreen and resolution changes behave correctly;
  (c) it builds and initialises on all three CI platforms; (d) the
  maintainer has verified it manually on Windows (P1), per `AGENTS.md` §4.
- **Stage 4 - revisit the fallback** in a later decision (see Open
  questions). Nothing in this ADR schedules its removal.

# Consequences

- **Positive.** A renderer on an actively developed API; the shader work
  that the fixed-function removal needs is done once instead of twice;
  the existing `RageDisplay` interface means the engine, themes and Lua
  are untouched; OpenGL as a fallback keeps machines without Vulkan
  working.
- **Negative.** Two real backends to maintain for as long as the fallback
  lives; Vulkan is much more code than OpenGL (memory, synchronisation,
  pipelines); macOS depends on MoltenVK, a translation layer; new build
  dependencies (Vulkan headers, a SPIR-V compiler, probably a memory
  allocator).
- **Risks.** Visual or timing differences between backends (mitigated by
  the screenshot gate); GPUs or VMs without Vulkan (mitigated by the
  fallback); MoltenVK missing a feature the engine needs (mitigated by the
  prototype stage); fixed-function features the interface exposes but the
  game barely uses (lighting, sphere mapping, cel shading) may be cheaper
  to remove from the interface than to emulate - to be decided from usage,
  not assumed.

# Open questions

1. **Vulkan baseline version.** *Resolved by the 2026-10-04 amendment
   below: target Vulkan 1.4.* Still open: which optional extensions the
   backend needs, and whether MoltenVK covers them on the two supported
   macOS releases (decided from the prototype).
2. **Fallback lifetime.** The fallback has no end date here. Whether and
   when OpenGL is retired should be decided after Vulkan has been the
   default for a release cycle, with real-world init-failure data.
3. **Which GL profile does the macOS fallback run on today?** Needs
   confirming before stage 3, because Apple's OpenGL support is frozen.
4. **Tooling choices.** SPIR-V compiler (glslang vs shaderc), memory
   allocator (Vulkan Memory Allocator or hand-written), and function
   loading (a loader library or direct linking). External dependencies
   follow the existing pattern (git submodule, pinned, verified).
5. **Vulkan in CI.** GitHub runners have no GPU. A software Vulkan
   implementation (Mesa lavapipe on Linux) could run a smoke and
   screenshot test; confirm it is workable. Windows and macOS CI would stay
   build-and-Null-backend only.
6. **Fixed-function interface cleanup.** Whether to shrink the `RageDisplay`
   interface (lighting, environment mapping, cel shading, combiner modes)
   before or during stage 2, based on what the shipped themes use.

# Amendment 2026-10-04 - target the latest Vulkan

Maintainer instruction: use the latest Vulkan version available. This
resolves open question 1 (baseline version).

- **Target: Vulkan 1.4**, the latest core version at the time of writing
  (1.4 was released 2024-12-02; the specification was at patch 1.4.365 on
  2026-10-02 and Vulkan SDK 1.4.357+ is current). The instance is created
  with `apiVersion = VK_API_VERSION_1_4`, and the backend is written against
  1.4 core: features promoted into core by 1.4 are used directly instead of
  being probed as extensions. The patch level is not pinned here; the SDK or
  header version is pinned as a build dependency in stage 1.
- **macOS:** MoltenVK 1.4 exposes Vulkan 1.4 over Metal, so the same target
  holds on macOS. MoltenVK is still a translation layer with documented
  limitations, so stage 1 must confirm that what this engine needs (2D and
  simple 3D, textures, render-to-texture, blend modes, screenshots) is
  covered, rather than assuming it.
- **Consequence for hardware:** a 1.4 baseline excludes GPUs and drivers
  that only expose an older Vulkan. They fall back to OpenGL automatically
  (see Decision), which makes the fallback more important than a lower
  baseline would. Which GPU generations actually expose 1.4 on Windows and
  Linux is an expectation to measure in stage 1, not a fact established
  here.
- **Trade-off accepted:** a lower baseline (1.3 or 1.2) would reach more
  GPUs and could be raised later; the maintainer chose the latest version
  and relies on the OpenGL fallback for older hardware.
- **Watch item:** the Vulkan SDK now also ships KosmicKrisp, a Vulkan
  implementation for Apple platforms. It is a possible alternative to
  MoltenVK; it is not evaluated or chosen here.
