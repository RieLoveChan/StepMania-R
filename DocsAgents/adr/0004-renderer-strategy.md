---
type: Architecture Decision
title: Renderer strategy
description: Overall direction for the rendering backend, and the fate of the D3D9 and GLES2 renderers, deferred from ADR 0001 §9.
tags: [adr, renderer, opengl, direct3d, proposed]
---

# Status

**Accepted** — 2026-09-15. Maintainer decision (slot reserved by ADR
0001 §9, 2026-09-02).

# Decision

- **Direction: migrate to GL 3.3 core profile.** Drop fixed-function
  entirely; `RageDisplay_OGL` moves to a modern shader-based pipeline.
  This is a real rewrite, not a cleanup pass — everything that assumes
  fixed-function state needs re-deriving.
- **Drop the D3D9 renderer (`RageDisplay_D3D`).** Windows keeps the GL
  path as its only backend going forward.
- **Drop `WITH_GLES2`.** Resolves `RageDisplay_GLES2.cpp`'s item-15
  `#if 0`/`#else` selector by deleting the disabled branch outright.

# Context

The engine currently ships four renderer backends:

- `RageDisplay_D3D` — Direct3D 9, Windows-only.
- `RageDisplay_OGL` — legacy (compatibility-profile) OpenGL, the main
  cross-platform path.
- `RageDisplay_GLES2` — OpenGL ES 2, used on Linux (`WITH_GLES2`).
- `RageDisplay_Null` — headless stub (used by `--SelfTest` /
  `sm_tests`).

ADR 0001 froze this area on purpose: "kept compiling, no investment, no
new features" until a direction is picked. Backlog item 20 (game-type
registry) and the RString/god-object work have kept the calendar full
since; this has not been revisited.

Two backlog-adjacent facts, not decisions:

- `RageDisplay_GLES2.cpp` is the one file in the whole `#if 0` sweep
  (item 15) that's an **active** `#if 0`/`#else` selector, not dead
  code — whichever way this ADR goes touches that file directly.
- `RageDisplay_D3D.cpp` still had a live `bugprone-integer-division`
  site fixed under item 12 this session — it's maintained, not
  abandoned, today.

# Consequences

Whatever gets decided here unblocks the two C++20-adjacent items ADR
0001 explicitly postponed pending this ADR (§9's closing note) and
gives item 15's `RageDisplay_GLES2.cpp` entry a real disposition
instead of "leave alone, it's an active selector."
