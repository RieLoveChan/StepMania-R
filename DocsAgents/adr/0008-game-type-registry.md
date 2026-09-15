---
type: Architecture Decision
title: Data-driven game-type registry
description: Replace the hand-maintained g_Games[]/g_Game_* C++ structs and the compile-time StepsType enum with a data-driven registry.
tags: [adr, game-types, stepstype, data-driven, proposed]
---

# Status

**Proposed** — backlog item 20, explicitly flagged there as
"deserves its own ADR when picked up." Not started.

# Context

Game types (dance, pump, bm, techno, ...) are defined today as
hand-written `static const Game g_Game_X = { ... }` struct literals in
`src/GameManager.cpp` (~150 lines each: controllers, button maps,
per-style mappings, menu buttons), registered in a hand-maintained
`g_Games[]` array. In parallel, note-chart types (`dance-single`,
`pump-double`, `bm-single7`, `pnm-nine`, ...) are a **compile-time
`StepsType` enum** with a matching `g_StepsTypeInfos[]` array.

Adding or editing a game today means editing a ~3600-line `.cpp` and
rebuilding — no config file, no runtime registration. All game types
are already *enabled* (`229d0769d5`); what's left is the *mechanism*.

**Already true, not part of what needs deciding:** `NoteSkins/` is
already the de facto on/off switch for which games reach the UI
(`GameManager::GetEnabledGames` → `IsGameEnabled` →
`NoteSkinManager::DoNoteSkinsExistForGame`) — a game with no
`NoteSkins/<name>/` skin is invisible regardless of `g_Games[]`. The
target end-state keeps that: `g_Games[]` becomes "every definition
found on disk," NoteSkins stays the enablement gate.

**Hard constraint, not a decision:** the on-disk `#STEPSTYPE` strings
are a stable contract with the ~20-year simfile library (`AGENTS.md`
§5). Every existing value must resolve identically after any change —
no mass cache invalidation, ever.

**Known correctness bug riding along:** `NoteSkins/Para/` is
capitalized but the actual game name is `para` — breaks on
case-sensitive filesystems (Linux). Whether this gets fixed as part of
this effort or as its own quick unrelated patch is question D below.

# Open questions

**A. Pursue now, or stay deferred?** Nothing else in the backlog
blocks on this.

**B. Data format.** Two real shapes, matching two patterns already
live elsewhere in the tree:

1. **Lua definition files** — matches how theme scripting already
   works; game/style/stepstype definitions become Lua tables loaded at
   startup.
2. **An ini-tree under `Games/`** — matches the `NoteSkins/`/`Themes/`
   pattern exactly (a directory per game, `.ini`-style definition
   files inside), arguably the more consistent choice given NoteSkins
   is already the enablement gate for the same games.

**C. Scope of the first cut.** The full effort is big — `StepsType`
(enum → runtime id) ripples through `NoteData`, `Style`, `Steps`,
`RadarValues`, score keepers, the editor, Lua bindings, and `Profile`
serialization, all at once if done in one pass. Two shapes for landing
it:

1. **One pass** — migrate `g_Games[]`/`g_Game_*` to data-driven *and*
   `StepsType` from enum to runtime id, together, since they're
   entangled (a `Game` definition references its supported
   `StepsType`s).
2. **Staged** — first make `g_Games[]` itself data-loaded while
   `StepsType` stays a compile-time enum (smaller, contained, doesn't
   touch `NoteData`/`RadarValues`/serialization at all), and defer the
   `StepsType` runtime-id migration to a deliberate follow-up ADR
   amendment once the first stage is proven.

**D. The `NoteSkins/Para/` capitalization bug.** Bundle the fix into
this effort (it's the same subsystem), or land it now as its own
small, unrelated patch regardless of what happens with (A)-(C)? This
one has no real downside to fixing immediately — it's a bug, not a
design choice — so it may not need to wait on this ADR at all.

# Consequences (of pursuing it)

This is the largest single piece of remaining backlog scope in the
whole modernization effort — bigger than any RString pilot or
god-object cluster so far, and the one item most likely to actually
need its own dedicated multi-session push rather than an opportunistic
pass.
