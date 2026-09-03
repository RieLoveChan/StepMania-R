---
type: Subsystem
title: Input pipeline
description: Path from a physical device event to a game button press seen by a Screen.
tags: [input, controls, mapping]
resource: src/
---

# Purpose

Turns raw device events into `GameButton` / `MenuButton` presses that
screens and `Player` consume, with remapping, debouncing, and
key-sequence ("code") detection.

# Pipeline

```
OS device
  -> src/arch/InputHandler/InputHandler_*   (raw scan, per platform)
  -> RageInput / RageInputDevice            (src/RageInput*.cpp; INPUTMAN)
  -> InputFilter                            (src/InputFilter.cpp; INPUTFILTER)
       debounce, repeat, held-time tracking; emits InputEvent
  -> InputMapper                            (src/InputMapper.cpp; INPUTMAPPER)
       DeviceInput -> GameInput (GameButton) using the mapping tables
  -> InputQueue / InputQueueCodes           (src/InputQueue*.cpp; INPUTQUEUE)
       records recent presses for code detection
  -> Screen::Input() / CodeDetector         active screen handles it
```

# Key files

| File | Role |
|---|---|
| `src/RageInput.*`, `src/RageInputDevice.*` | Device registry + event source |
| `src/InputFilter.*` | Debounce/repeat; `INPUTFILTER` global |
| `src/InputMapper.*` | Device→game button mapping; load/save mappings; `INPUTMAPPER` |
| `src/InputQueue.*`, `src/InputQueueCodes.*` | History buffer for sequences |
| `src/CodeDetector.*`, `src/CodeSet.*` | Named codes (e.g. modifiers, menu shortcuts) |
| `src/GameInput.*`, `src/GameConstantsAndTypes.*` | `GameButton` / `GameController` enums |
| `src/InputEventPlus.h` | Event struct passed to screens |
| `src/ScreenMapControllers.cpp` | The remap UI |

# Gotchas

- Mappings are per-**Game** (`dance`, `pump`, …); `GameManager` defines
  each game's buttons. Default maps come from the game definition, user
  overrides are saved to `Data/`.
- `MenuButton` vs `GameButton`: menu navigation is a fixed mapping layer
  on top of game buttons.
- Adding a new binding surface usually means touching `InputMapper`
  (tables), `GameManager` (button list), and the theme's key-config UI.
- Code sequences (e.g. `LRLR`) are matched in `InputQueue` against
  `CodeSet` entries; timing window matters.
- Platform-specific device quirks belong in `src/arch/InputHandler/`, not
  here — see [arch.md](./arch.md).
