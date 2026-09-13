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
- 2026-09-13 -- sixth split: GameState's MultiPlayer-mode fields into
  GameStateMultiPlayerData.h (header-only). First cluster that is NOT
  a single contiguous block in the header: m_MultiPlayerStatus,
  m_bMultiplayer/m_iNumMultiplayerNoteFields, and m_pMultiPlayerState
  sit at three separate spots, interleaved with unrelated members and
  with the core 2-player fields (m_bSideIsJoined, m_pPlayerState) that
  were deliberately left alone as too foundational. **The fields don't
  need to be textually contiguous** -- each one becomes a reference
  member in its own original location, as long as the new component
  itself is declared before all of them (place it where the first
  field in declaration order used to sit). Scouted the real blast
  radius first (grep -rn, not a glob) before committing: ~34 sites
  across 9 files, but only 2 files had more than 1-2 touches each.
  **New kind of member handled: a heap-allocated pointer array**
  (m_pMultiPlayerState, `new PlayerState` per slot in the constructor
  *body*, `SAFE_DELETE` in the destructor -- both loops already live
  outside the member-initializer list). The reference member only
  needs binding in the init-list; the existing allocation/deallocation
  loops needed zero changes, since new/delete/indexing all pass
  through a reference-to-array exactly like the original array. A
  moved method (IsMultiPlayerEnabled) that a *free function* elsewhere
  calls through the god object's public interface
  (GetNextEnabledMultiPlayer calling GAMESTATE->IsMultiPlayerEnabled)
  also needed zero changes, since the public method signature never
  changed, only what's behind it.
- 2026-09-13 -- seventh split: GameState's "used by themes that
  support heart rate entry" fields (m_DanceStartTime, m_DanceDuration)
  into GameStateDanceData.h (header-only). Pure data, tiny external
  footprint (only ScreenGameplay.cpp). Confirms a wrinkle for
  class-type members: m_DanceStartTime is a RageTimer, a class type,
  so even though it was never in GameState's own constructor
  init-list, it was still being correctly default-constructed all
  along (class-type members always get default-constructed unless
  explicitly initialized, unlike POD members which are left
  uninitialized). The new component's own implicit default constructor
  reproduces this automatically -- just don't accidentally give the
  new component an explicit constructor that does something different
  for that member.
- 2026-09-13 -- eighth split: GameState's PLAY_MODE_BATTLE/
  PLAY_MODE_RAVE fields (m_fOpponentHealthPercent,
  m_fTugLifePercentP1) into GameStateBattleRaveData.h (header-only).
  Pure data (two floats), no methods, no hard boundaries across the 6
  touching files. Neither field was in GameState's original
  constructor init-list, so the new component correctly has no
  explicit constructor -- same shape as Haste, no new wrinkle. This
  cluster mainly confirms the pattern generalizes cleanly once a
  candidate's usage is plain reads/writes: the scouting + verification
  steps are now fully mechanical for this shape of field.
- 2026-09-13 -- ninth split: GameState's per-game/round random seed
  fields (m_iGameSeed, m_iStageSeed both int, m_sStageGUID an RString)
  plus SetNewStageSeed() into GameStateStageSeedData.h (header-only).
  New wrinkle: a moved *method* (SetNewStageSeed) that only touches
  the moved fields plus a free function (rand()) moves as a real
  implementation with zero dependency on GameState singletons at all
  -- simpler than the Attract-cluster case (which needed PREFSMAN/
  CommonMetrics) or the Workout-cluster case (which needed PROFILEMAN/
  STATSMAN). Also confirms clusters don't have to be pure item-9 work:
  m_sStageGUID is an RString field, but it stays RString here since
  this is a god-object split, not an item-10 RString migration -- the
  two efforts are independent and a field doesn't need both done at
  once. Section-5-adjacent (NoteDataUtil.cpp, Course.cpp read
  m_iStageSeed for shuffle/seed math) -- re-verified [corpus] and
  [crs] tags unchanged.
- 2026-09-13 -- tenth split: GameState's "character stuff" field
  (m_pCurCharacters[NUM_PLAYERS], a Character* array) into
  GameStateCharacterData.h (header-only), using the same array
  reference-to-array technique as cluster 6's m_MultiPlayerStatus
  (Character* (&m_pCurCharacters)[NUM_PLAYERS];). No new wrinkle --
  confirms the array-reference technique is now routine for any
  single-array field with a small, hard-boundary-free external
  footprint.
- 2026-09-13 -- eleventh split: GameState's own private "Timing
  position corrections" fields (m_LastPositionTimer, an RageTimer;
  m_LastPositionSeconds, a float; m_paused, a bool) into
  GameStatePositionCorrectionData.h (header-only). First cluster whose
  fields were already `private` inside GameState itself, not merely
  externally under-exposed -- confirms private god-object fields are
  fair game too, and are if anything *safer* candidates since there's
  no possibility of an external caller ever depending on them
  directly. Both touching methods (ResetMusicStatistics,
  UpdateSongPosition) also read/write unrelated GameState state in the
  same body, so neither moved as a real implementation -- they stay on
  GameState and read through the reference members exactly like
  before. None of the three fields were in the original constructor
  init-list, so no explicit constructor needed on the new component.
- 2026-09-13 -- twelfth split: GameState's MusicWheel expanded/last-
  open section fields (sExpandedSectionName, sLastOpenSection, both
  RString) into GameStateSectionData.h (header-only). A trap avoided:
  WheelBase.h and MusicWheel.cpp each declare their own *same-named*
  local member m_sExpandedSectionName on the wheel classes themselves
  -- a completely different field that happens to share a name with
  GameState's. Only grep hits qualified with GAMESTATE-> (or accessed
  through a GameState* in GameState.cpp itself) are the real external
  footprint; unqualified m_sExpandedSectionName hits belong to the
  wheel class and must be excluded by hand, not just text-matched.
  After this cluster, a full fresh scan of GameState.h found no more
  low-risk candidates: remaining fields are either already-rejected
  coupled clusters (Ranking Stuff, Award stuff, the stage-token
  cluster) or single isolated bools with no natural thematic grouping
  (m_bDopefish, m_bLoadingNextSong, m_bBackedOutOfFinalStage,
  m_bTemporaryEventMode) -- forcing those into one artificial
  component would be exactly the kind of unneeded abstraction this
  project avoids, so phase 1's easy/safe clusters are exhausted for
  now without a maintainer decision on how (or whether) to group the
  leftovers.
