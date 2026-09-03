---
type: Subsystem
title: Screens (state machine)
description: ScreenManager and the Screen* classes that make up every UI state.
tags: [screens, ui, state-machine]
resource: src/
---

# Purpose

A `Screen` is one top-level UI state (title menu, select music, gameplay,
evaluation, options, editor…). `ScreenManager` (`SCREENMAN`) owns the
stack and transitions. File grouping: `src/CMakeData-screen.cmake`
(Gameplay, Options, Others) — ~60 `Screen*` classes.

# Lifecycle

1. `SCREENMAN->SetNewScreen("ScreenName")` — name is resolved through
   **theme metrics** (`[ScreenName]` section: `Class`, `NextScreen`,
   `PrevScreen`, timer, elements…).
2. The C++ class named by `Class=` is constructed; `Init()` builds actors
   from the theme.
3. `BeginScreen()` → per-frame `Update()` / `Input()` / `Draw()`.
4. `HandleScreenMessage(SM_*)` drives internal transitions; `SM_GoToNextScreen`
   reads `NextScreen` metric.

# Important classes

| Class | Role |
|---|---|
| `Screen` | Base: message queue, input, tween-in/out, lifetime |
| `ScreenWithMenuElements` | Adds header/footer/help/timer/background — most menus derive from this |
| `ScreenManager` | `SCREENMAN`; stack, `SystemMessage`, screen reload, shared-background |
| `ScreenSelectMusic` | Song wheel; heavy interaction with `SONGMAN`, `GAMESTATE` |
| `ScreenGameplay*` | The play screen — see [gameplay.md](./gameplay.md) |
| `ScreenOptions` + `ScreenOptions*` / `OptionRow*` | Generic option-list framework |
| `ScreenEdit`, `ScreenEditMenu` | The step editor |
| `ScreenEvaluation` | Post-song results |
| `ScreenDebugOverlay`, `ScreenSyncOverlay`, `ScreenStatsOverlay`, `ScreenSystemLayer` | Always-on overlays |
| `ScreenPrompt`, `ScreenMiniMenu`, `ScreenTextEntry` | Reusable modal helpers |

# Gotchas

- **Screen flow lives in theme metrics, not C++.** To change what comes
  after a screen, edit the theme's `metrics.ini`, not the class.
- Adding a screen: new `Screen*` class + `REGISTER_SCREEN_CLASS`, then a
  `[MyScreen]` metrics block in the theme(s).
- `ScreenMessage` values (`SM_*`) are defined in `ScreenMessage.h` /
  per-screen; collisions are a real hazard — use the screen-local range.
- Overlays are managed by `ScreenManager`, not pushed like normal screens.
- `ScreenSelectMaster` / `ScreenSelect*` are data-driven from metrics for
  simple choice menus — often no new class is needed.
