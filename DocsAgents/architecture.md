---
type: Reference
title: StepMania engine architecture
description: High-level map of startup, the main loop, and how the major layers fit together.
tags: [architecture, overview]
---

# Layers (bottom to top)

| Layer | What | Concept |
|---|---|---|
| Platform drivers | Per-OS implementations of window, input, sound, movie, lights, threads | [subsystems/arch.md](./subsystems/arch.md) |
| Rage engine | `Rage*` classes: file I/O, display (GL/D3D), sound mixing, math, timers, logging | [subsystems/rage.md](./subsystems/rage.md) |
| Global singletons | `*Manager` objects + `GameState` — the service locator | [subsystems/singletons.md](./subsystems/singletons.md) |
| Data model | `Song`/`Steps`/`NoteData`/`TimingData`/`Style`/`Profile` + loaders | [subsystems/data-structures.md](./subsystems/data-structures.md), [subsystems/simfile-formats.md](./subsystems/simfile-formats.md) |
| Actors | Scene-graph: `Actor` → `ActorFrame` → concrete actors, tween/draw | [subsystems/actors.md](./subsystems/actors.md) |
| Screens | State machine; one active `Screen` owns the current UI | [subsystems/screens.md](./subsystems/screens.md) |
| Gameplay | `Player`/`NoteField`/scoring, driven by `ScreenGameplay` | [subsystems/gameplay.md](./subsystems/gameplay.md) |
| Lua + theme | Themes script screens/actors via Lua; metrics parametrize them | [subsystems/lua.md](./subsystems/lua.md), [subsystems/theming.md](./subsystems/theming.md) |

# Startup (`src/StepMania.cpp`)

`main()` → `sm_main()`. Managers are `new`ed in a **fixed order** because
of hard dependencies; `ShutdownGame()` deletes them in roughly reverse
order (see `src/StepMania.cpp:301-337` for the canonical teardown list —
it doubles as a dependency cheat-sheet). Rough creation order:

1. `HOOKS` (ArchHooks), `LUA`, `FILEMAN`, `LOG`
2. `PREFSMAN` → `GAMESTATE` → `SOUNDMAN` / `SOUND`
3. `GAMEMAN`, `THEME`, `ANNOUNCER`, `NOTESKIN`
4. `DISPLAY` (via `ApplyGraphicOptions`), `TEXTUREMAN`, `MODELMAN`, `FONT`
5. Input stack: `INPUTFILTER` → `INPUTMAPPER` → `INPUTQUEUE` → `INPUTMAN`
6. `SONGMAN`, `PROFILEMAN`, `CRYPTMAN`, `UNLOCKMAN`, `MEMCARDMAN`
7. `MESSAGEMAN`, `STATSMAN`, `SCREENMAN` — then the first screen is set

If you add a manager, insert it in both the create sequence and the
`ShutdownGame()` delete sequence, respecting dependencies.

# Main loop (`src/GameLoop.cpp`)

`GameLoop::RunGameLoop()` runs until quit. Each frame:

1. `CheckFocus()` — handle app focus / theme-or-game change requests.
2. `GameLoop::UpdateAllButDraw()`:
   - compute `fDeltaTime` (scaled by `SetUpdateRate`)
   - `SOUNDMAN->Update` → `SOUND->Update` → `TEXTUREMAN->Update`
   - `GAMESTATE->Update` → `SCREENMAN->Update` → `MEMCARDMAN->Update`
   - `HandleInputEvents()` — pumps `INPUTFILTER`, routes to the top screen
   - `LIGHTSMAN->Update`
3. `SCREENMAN->Draw()` — the active screen draws its actor tree.

Concurrent/threaded rendering path exists
(`StartConcurrentRendering`/`FinishConcurrentRendering`) for loads.

# Control flow at runtime

- `SCREENMAN` (`ScreenManager`) owns a stack of screens; `SetNewScreen`
  swaps the top. Screen names and transitions are defined in **theme
  metrics**, not C++.
- Input events flow device → `RageInput` → `InputFilter` (debounce) →
  `InputMapper` (device button → `GameButton`) → `InputQueue` (for codes)
  → active `Screen::Input()`.
- Screens read layout/behavior from `THEME->GetMetric(...)` and build
  actors from theme Lua/XML via `ActorUtil::LoadFromNode` /
  `ActorUtil::MakeActor`.

# Where things live

| Want to change… | Look at |
|---|---|
| A menu's layout or text | `Themes/<theme>/` (metrics + Lua), not `src/` |
| How notes scroll / judge | [subsystems/gameplay.md](./subsystems/gameplay.md) |
| A new simfile field | [subsystems/simfile-formats.md](./subsystems/simfile-formats.md) + `TimingData`/`Song`/`Steps` |
| A new Lua function for themes | [subsystems/lua.md](./subsystems/lua.md) |
| Rendering / new actor type | [subsystems/actors.md](./subsystems/actors.md) |
| OS-specific bug | [subsystems/arch.md](./subsystems/arch.md) |
