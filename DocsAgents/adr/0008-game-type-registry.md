---
type: Architecture Decision
title: Data-driven game-type registry
description: Replace the hand-maintained g_Games[]/g_Game_* C++ structs and the compile-time StepsType enum with a data-driven registry.
tags: [adr, game-types, stepstype, data-driven, proposed]
---

# Status

**Accepted** — 2026-09-15. Maintainer decision. Backlog item 20.

**Stage 1 implemented and closed — 2026-09-16.** All 12 games migrated
to `Games/<name>/` ini files (`src/GameDataIO.h`/`.cpp`), `GameManager::
LoadGames()` reads that tree at startup, and the old `g_Games[]`/
`g_Game_*`/`g_Style_*`/`g_AutoKeyMappings_*` C++ literals are deleted
(`dd56cf3d91`, ~3600 lines removed from `GameManager.cpp`). Verified via
a field-for-field round-trip characterization test
(`tests/test_GameDataIO.cpp`) run *before* the literals were deleted, the
full `sm_tests` suite (231 cases) and `--SelfTest` both before and after
deletion, and green CI on Windows/macOS/Ubuntu for both the migration
commit and the deletion commit. `StepsType` (stage 2, deferred per this
ADR's own scope below) is untouched. See `modernization-backlog.md` item
20 for the up-to-date detail.

# Decision

- **Staged, not one-pass.** This ADR covers **stage 1 only**: make
  `g_Games[]`/`g_Game_*` data-driven. `StepsType` stays a compile-time
  enum for now — `NoteData`/`Style`/`Steps`/`RadarValues`/score
  keepers/the editor/Lua bindings/`Profile` serialization are **not**
  touched in this stage. The runtime-id migration for `StepsType` is
  deferred to a future ADR amendment once stage 1 is proven.
- **Format: an ini-tree under `Games/`**, matching the `NoteSkins/`/
  `Themes/` pattern already live in the tree — one folder per game,
  an `.ini`-style definition file inside. `NoteSkins/` remains the
  enablement gate exactly as it is today; `g_Games[]` becomes "every
  definition found on disk" instead of a hand-maintained array.
- **The `NoteSkins/Para/` capitalization bug is fixed independently**
  (2026-09-15, does not wait on this ADR) — see the fix commit near
  this ADR's landing.

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

# Consequences

This is the largest single piece of remaining backlog scope in the
whole modernization effort — bigger than any RString pilot or
god-object cluster so far, and the one item most likely to actually
need its own dedicated multi-session push rather than an opportunistic
pass.
