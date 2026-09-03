---
type: Subsystem
title: Global singletons
description: The *Manager objects and GameState — the engine's service locator, their globals, lifetimes, and init order.
tags: [singletons, managers, gamestate, lifecycle]
resource: src/
---

# Purpose

StepMania uses global singletons as a service locator. Each is `new`ed in
`src/StepMania.cpp` at startup and reachable through an ALL-CAPS pointer
declared in its header. File grouping: `src/CMakeData-singletons.cmake`.

# The globals

| Global | Class | Responsibility |
|---|---|---|
| `GAMESTATE` | `GameState` | **The** mutable runtime state: current game/style, players, selected song/steps/course, play mode, options, timing. Read/written everywhere. |
| `SCREENMAN` | `ScreenManager` | Screen stack + transitions. See [screens.md](./screens.md). |
| `THEME` | `ThemeManager` | Metrics + asset path resolution. See [theming.md](./theming.md). |
| `PREFSMAN` | `PrefsManager` | `Preferences.ini` values. |
| `SONGMAN` | `SongManager` | Loads/holds all `Song`/`Course` objects. |
| `PROFILEMAN` | `ProfileManager` | Player profiles, scores, settings persistence. |
| `GAMEMAN` | `GameManager` | Game defs (`dance`/`pump`/…), styles, note-skin-agnostic button lists. |
| `NOTESKIN` | `NoteSkinManager` | See [noteskins.md](./noteskins.md). |
| `INPUTFILTER`/`INPUTMAPPER`/`INPUTQUEUE`/`INPUTMAN` | input stack | See [input.md](./input.md). |
| `SOUND` / `SOUNDMAN` | `GameSoundManager` / `RageSoundManager` | Music + SFX / low-level mixer. |
| `SCREENMAN`,`STATSMAN`,`MESSAGEMAN` | Stats + `Message` pub/sub | `MESSAGEMAN` broadcasts named messages to Lua/actors. |
| `LUA` | `LuaManager` | Lua state. See [lua.md](./lua.md). |
| `FILEMAN` | `RageFileManager` | VFS. |
| `DISPLAY`/`TEXTUREMAN`/`MODELMAN`/`FONT` | Rage render globals | See [rage.md](./rage.md). |
| `CRYPTMAN`,`UNLOCKMAN`,`MEMCARDMAN`,`BOOKKEEPER`,`ANNOUNCER`,`CHARMAN`,`LIGHTSMAN`,`IMAGECACHE`,`SONGINDEX`,`NETWORK` | secondary managers | crypto, unlocks, memory cards, coin bookkeeping, announcer, characters, cabinet lights, banner cache, song cache DB, netplay |

# Init / shutdown order

Defined by dependencies in `src/StepMania.cpp`. `ShutdownGame()` (~line
301) deletes in reverse — treat that list as the dependency graph. See
[../architecture.md](../architecture.md#startup-srcstepmaniacpp).

# Gotchas

- **`GAMESTATE` is a god object.** Most feature work reads or mutates it.
  Be careful about *when* fields are valid (e.g. `m_pCurSteps` only
  during/after selection).
- Singletons are not lazily created — using one before its `new` in
  `StepMania.cpp` crashes. Order matters.
- Many managers expose Lua tables of the same name (`GAMESTATE`, `THEME`,
  `PREFSMAN`, `SCREENMAN`) — a C++ change often needs a matching binding
  update.
- Tests in `src/tests/` that need a manager must stand it up manually.
