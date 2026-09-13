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

- Windows build green + `--SelfTest` smoke.
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
- 2026-09-13 — first split done: `GameState`'s "Edit stuff" cluster
  (8 members + `GetEditLocalProfile()`) into `GameStateEditData.h`/`.cpp`.
  **Key technique for zero call-site churn on raw public data members**
  (this codebase doesn't wrap most `GameState` fields in accessors —
  they're touched directly as `GAMESTATE->m_xxx` everywhere): declare
  the new component as a normal value member (`GameStateEditData
  m_EditData;`), then declare **reference members** in the god object
  for each moved field (`bool& m_bIsUsingStepTiming;`,
  `BroadcastOnChange<StepsType>& m_stEdit;`, etc.), bound in the
  constructor's mem-initializer list to `m_EditData.xxx`. Every
  existing call site — reads, writes, `.Set()` calls, even the
  `Luna<GameState>` Lua-binding block inside `GameState.cpp` itself —
  keeps compiling and behaving identically, because a reference member
  transparently forwards to its referent. This only works because (a)
  `m_EditData` is declared *before* the reference members in the class
  body, so it's fully constructed by the time they bind to it, and
  (b) the god object's lifetime is single and non-copied (a reference
  member would dangle if the object were ever copied/moved — verify
  the god object's copy ctor/assignment are already deleted or
  equivalently unused before relying on this). Reset/init parity
  (the gotcha above) came free: `Reset()`'s existing lines writing to
  the moved fields were never touched, they still write through the
  reference to the same storage. Full gate green (`sm_tests`, `ctest`,
  Release, `--SelfTest`); no live gameplay spot-check was performed for
  this specific commit (data-only extraction, no logic moved beyond a
  straight cut-paste of `GetEditLocalProfile()`'s body) — a maintainer
  spot-check of the editor is still the confirming step per this
  playbook's own Verification section.
- 2026-09-13 — second split: `GameState`'s "used in workout" fields
  into `GameStateWorkoutData.h`/`.cpp`. Same reference-member technique,
  one new wrinkle: **an array member needs a reference-to-array
  declarator**, `T (&name)[N]`, not `T& name[N]` (which doesn't parse —
  arrays of references aren't a thing) — e.g.
  `bool (&m_bGoalComplete)[NUM_PLAYERS];`. It binds and indexes exactly
  like the original array, so `m_bGoalComplete[p] = ...` call sites
  need zero changes, same as the scalar case. Also confirmed: a moved
  method only needs to become a *real* implementation on the new
  component (not just a thin forward) when it doesn't touch any other
  god-object state — check this before moving a method body, not just
  its owning members; `GetGoalPercentComplete()` qualified (only used
  `PROFILEMAN`/`STATSMAN` + its own parameter), so it moved as-is.
- 2026-09-13 — third split: `GameState`'s "Attract stuff" into
  `GameStateAttractData.h`/`.cpp`. Confirms a lesson from the RString
  playbook applies here too: a cluster that *looks* small and cohesive
  by name (`GameState.h`'s "Award stuff" comment) can still have a much
  bigger real blast radius if its types (`StageAward`/`PeakComboAward`
  here) are used broadly elsewhere in the codebase — grep every
  candidate's full footprint (types included, not just the god
  object's own members) before committing to it. No new wrinkle this
  time (plain `int`, not an array or a `BroadcastOnChange<T>`) —
  straightforward reference member.
- 2026-09-13 — fourth split: `GameState`'s "Autogen stuff" into
  `GameStateAutogenData.h`. **A header-only component is fine** when
  the whole thing is one inline getter + one member (no `.cpp` needed —
  don't create an empty translation unit just for symmetry with
  earlier clusters). Also: a `Luna<GodObject>` Lua-binding thunk that
  calls container methods directly on the moved member
  (`p->m_autogen_fargs.push_back(...)`, `.size()`, `operator[]`) needs
  zero changes under the reference-member technique, same as any other
  call site — containers support all their normal operations through a
  reference exactly like through the original value.
- 2026-09-13 — fifth split: `GameState`'s "Haste" fields (3 plain
  floats, no methods at all) into `GameStateHasteData.h` (header-only
  again). **Check whether the original fields were ever in the god
  object's constructor init-list before deciding whether the new
  component needs its own constructor.** These three weren't (left
  implicitly uninitialized until the first `Reset()` call, same as
  most plain-scalar `GameState` members) — giving `GameStateHasteData`
  a constructor that zero-initializes them would have silently changed
  behavior versus the original (a real, if minor, correctness risk:
  reference members MUST be bound in the god object's init-list either
  way, but what value the *referent* starts with is a separate
  question you have to check, not assume).
