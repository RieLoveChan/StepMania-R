---
type: Playbook
title: Carve a slice out of a god object
description: Extract a cohesive cluster of members from GameState / GameManager / ScreenEdit into an owned component, with zero call-site churn in phase 1.
tags: [modernization, refactor, gamestate, god-object]
---

# Goal

Reduce a 3k+ LOC god class by moving one cohesive group of
members+methods into its own type, **without** changing the ~thousands of
call sites in the same PR and without behavior change.

# When to use

`GameState` (~3.5k LOC, ~112 members, ~2,100 `GAMESTATE->` call sites,
`LunaGameState` binding at `GameState.cpp:2735-3497`), `GameManager`
(~3.6k), `ScreenEdit` (~6.6k), `Profile` (~2.9k). High value, high risk —
always a `AGENTS.md` §4 large change.

# Strategy: two phases, two PRs

**Phase 1 — extract, keep the facade.** The god object gains a member of
the new type; existing accessors delegate to it. Call sites are
untouched. Fully behavior-preserving, mechanically reviewable.

**Phase 2 (optional, later) — migrate call sites** in bounded batches to
talk to the component directly, one caller subsystem per PR.

Do **not** attempt both in one PR.

# Files touched (phase 1)

| Path | Change |
|---|---|
| `src/<NewComponent>.h` / `.cpp` | New type holding the moved members + logic |
| `src/<GodObject>.h` / `.cpp` | Hold a `<NewComponent>` member; existing getters/setters delegate |
| `src/CMakeData-*.cmake` | Register the new TU in the right `source_group` list |
| `src/<GodObject>.cpp` `Luna*` block | Keep the Lua method names; forward to the component |
| `DocsAgents/subsystems/singletons.md` (for GameState) | Note the new seam |

# Steps

1. **Find a cohesive cluster.** Members that are read/written together and
   share a concept (e.g. in `GameState`: the edit-mode fields, or the
   coin/credit bookkeeping, or the multiplayer/`PlayerNumber` fan-out).
   `rg -n "m_\w+" src/GameState.h` then group by prefix/comment blocks.
2. **Create `<NewComponent>`** with those members. Move the small helper
   methods that only touch them. Plain owned object (no singleton).
3. **Add it to the god object** as a member (`m_<component>` /
   `m_p<component>`). Construct/reset it where the old members were
   initialized (`GameState::Reset*` paths matter — mirror them exactly).
4. **Delegate:** every existing public accessor that touched a moved
   member now forwards: `int GetX() const { return m_component.GetX(); }`.
   Keep signatures identical. Call sites do not change.
5. **Lua:** in the `Luna<GodObject>` block, the thunks that used the moved
   members now call through the component. **Method names stay the same** —
   themes must not notice.
6. **Register** the new files in `src/CMakeData-*.cmake`.
7. Build Windows, diff-review: the only semantic change should be "these
   fields now live one indirection away".

# Gotchas

- **Reset/init parity.** `GameState` has multiple reset scopes
  (`Reset`, `ResetPlayer`, per-stage). A moved member must be
  reset/initialized at exactly the same points, or you get stale state
  across songs — a classic hard-to-spot regression.
- **Serialization.** If any moved member is written to `Profile` / stats
  XML / netplay, the read/write code moves or delegates too, and the
  on-disk format must be byte-identical.
- **`LunaGameState` is ~760 lines.** Grep it for every moved member name
  before you start; missing one silently breaks a theme.
- **Copy semantics.** `GameState` may be copied/snapshotted in places
  (edit mode, sync). Give the component correct copy behavior.
- Don't "improve" the moved code in the same PR (no signature changes, no
  RString migration, no renames) — keep the diff reviewable.
- Order-of-init: a component constructed in the god object's init list
  cannot depend on later-constructed singletons.

# Verification

- Windows build green + headless smoke test (once it exists).
- **Always higher-risk.** Phase 1 and phase 2 are separate commits (and
  ideally a short-lived branch) — a behavior-preserving extract that
  turns out not to be behavior-preserving is the worst case, and one
  revert must cleanly undo it.
- Spot-check to spell out in the message: play a song, enter/exit edit
  mode, switch styles/players, evaluate — every path touching the moved
  cluster. Then commit + push and tell the maintainer it landed.

# History

- 2026-09-02 — created from recon (`GameState.h` ~112 members,
  `GameState.cpp` `LunaGameState` span). No split done yet — first
  executor records which cluster was taken and the reset-parity notes.
