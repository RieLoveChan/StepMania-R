---
type: Architecture Decision
title: Renderer strategy
description: Overall direction for the rendering backend, and the fate of the D3D9 and GLES2 renderers, deferred from ADR 0001 §9.
tags: [adr, renderer, opengl, direct3d, proposed]
---

# Status

**Proposed** — slot reserved by ADR 0001 §9 (2026-09-02), not yet
written. This file exists so the open questions have one place to be
answered, not to pre-judge them.

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

# Open questions

**A. Overall direction.** Three real options, not mutually exclusive
in sequence but the first choice shapes what "clean up" even means:

1. **Clean up legacy GL as-is** — keep the current compatibility-profile
   OpenGL renderer as the cross-platform baseline, fix its rough edges
   (immediate-mode leftovers, fixed-function pipeline quirks), no new
   API adopted.
2. **Move to GL 3.3 core profile** — drops fixed-function entirely,
   modern shader-based pipeline, but a real rewrite of `RageDisplay_OGL`
   and everything that assumes fixed-function state.
3. **Adopt an abstraction layer** (bgfx or similar) — one backend code
   path targets D3D/Vulkan/GL/Metal underneath; biggest rewrite, but
   removes the multi-backend maintenance burden going forward (no more
   hand-written D3D9 *and* GL *and* GLES2 paths).

**B. Drop the D3D9 renderer (`RageDisplay_D3D`)?** Windows-only, and
Windows already has a fully-supported GL path — so this is "one less
backend to maintain" against "some Windows users may rely on D3D9 for
GPU/driver compatibility GL doesn't have." Depends on (A): a bgfx-style
abstraction layer would likely replace D3D9 with a D3D11/12 path
instead of just deleting it.

**C. Drop `WITH_GLES2`?** Linux-only, P3 priority per `AGENTS.md` §3 —
the lowest-cost drop candidate of the three backends if the direction
in (A) doesn't need it. Currently the *only* file in the codebase with
a live `#if 0`/`#else` runtime selector (item 15) — dropping it means
deleting the `#if 0` branch outright rather than leaving it alone.

# Consequences (of any answer)

Whatever gets decided here unblocks the two C++20-adjacent items ADR
0001 explicitly postponed pending this ADR (§9's closing note) and
gives item 15's `RageDisplay_GLES2.cpp` entry a real disposition
instead of "leave alone, it's an active selector."
