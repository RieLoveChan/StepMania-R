---
type: Playbook
title: Add a new Screen
description: Add a new Screen* state class and wire it into the theme's screen flow.
tags: [screens, ui, metrics]
---

# Goal

Add a new top-level UI state (a `Screen*` class) that the engine can
navigate to.

# When to use

You need behavior a data-driven `ScreenSelectMaster` / `ScreenMiniMenu` /
`ScreenPrompt` cannot express. **First check** whether an existing
generic screen + a metrics block is enough — most "new menus" need no C++.

# Files always touched

| Path | Change |
|---|---|
| `src/Screen<Name>.h` / `.cpp` | New class, derived from `Screen` or (usually) `ScreenWithMenuElements` |
| `src/Screen<Name>.cpp` | `REGISTER_SCREEN_CLASS( Screen<Name> );` at file scope |
| `src/CMakeData-screen.cmake` | Add both files to the right `SMDATA_SCREEN_*` list + `source_group` |
| Theme `metrics.ini` (`Themes/_fallback/metrics.ini` at least) | `[Screen<Name>]` block: `Class`, `ScreenType`, elements, `NextScreen`, `PrevScreen`, timer |

# Steps

1. Create `src/Screen<Name>.{h,cpp}`. Derive from `ScreenWithMenuElements`
   for a standard menu (header/footer/help/background come free).
2. Implement `Init()` (build actors, read metrics), and as needed
   `BeginScreen()`, `Input()`, `HandleScreenMessage()`, `Update()`.
3. Add `REGISTER_SCREEN_CLASS( Screen<Name> );` at file scope in the `.cpp`.
4. Register the translation units in `src/CMakeData-screen.cmake`
   (mirror how a neighbouring screen is listed, `_SRC` and `_HPP`).
5. Add a `[Screen<Name>]` section to `Themes/_fallback/metrics.ini`
   (and any shipped theme that needs it). At minimum:
   `Class="Screen<Name>"`. Wire `NextScreen`/`PrevScreen` for flow.
6. Route to it: from Lua (`SCREENMAN:SetNewScreen("Screen<Name>")`), from
   a metrics `NextScreen`, or a `GameCommand` `screen,Screen<Name>`.

# Gotchas

- **Screen flow lives in metrics, not C++.** `NextScreen` / `PrevScreen`
  and `SM_GoToNextScreen` read the theme. Do not hard-code transitions.
- Forgetting the `CMakeData-screen.cmake` entry = "unresolved external"
  or the class simply never registers. Forgetting the metrics block =
  `RageException` "Screen ... is missing" at runtime.
- `ScreenMessage` (`SM_*`) collisions: use the screen-local enum range,
  not raw ints, and not another screen's values.
- Overlays (stats/sync/debug/system) are managed by `ScreenManager`, not
  navigated to like normal screens — different mechanism.
- New screens need theme assets; against `_fallback` they will look
  bare. That is fine for engine work.

# Verification

- Windows build green (+ headless smoke test once it exists), then
  commit + push (`AGENTS.md` §4).
- Spot-check to note in the commit: navigate to the screen in-game,
  exercise input, confirm transitions in and out.
- Higher-risk (new class + build wiring + metrics + a navigable state) —
  split into separate commits (class, CMake wiring, metrics) so a revert
  is surgical.

# History

- 2026-09-02 — created from `subsystems/screens.md` recon:
  `REGISTER_SCREEN_CLASS` confirmed in `src/ScreenTitleMenu.cpp:20`;
  grouping in `src/CMakeData-screen.cmake`. Not yet executed end-to-end.
