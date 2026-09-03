# Subsystems

Each concept below maps a slice of `src/` to its purpose, entry points,
and gotchas. The project's own translation-unit grouping lives in
`src/CMakeData-*.cmake` — those files are the ground truth for "which
files belong together".

# Engine core

* [rage](./rage.md) - Low-level engine layer (`Rage*`): files, display, sound, threads, math.
* [arch](./arch.md) - Per-OS driver implementations under `src/arch/` and `src/archutils/`.
* [input](./input.md) - Device-to-game-button input pipeline.

# Scene and UI

* [actors](./actors.md) - Actor scene-graph and rendering primitives.
* [screens](./screens.md) - Screen state machine and `Screen*` classes.
* [singletons](./singletons.md) - Global `*Manager` objects and `GameState`.

# Content model

* [data-structures](./data-structures.md) - `Song`, `Steps`, `NoteData`, `TimingData`, `Style`, `Profile`, scoring.
* [simfile-formats](./simfile-formats.md) - Loaders/writers for `.sm` `.ssc` `.dwi` `.bms` `.ksf` + cache.

# Play and presentation

* [gameplay](./gameplay.md) - `Player`, `NoteField`, `NoteDisplay`, judgment, life, scoring.
* [lua](./lua.md) - Lua 5.1 binding layer and theme scripting API.
* [theming](./theming.md) - `ThemeManager`, metrics, `Themes/` tree, localization.
* [noteskins](./noteskins.md) - `NoteSkinManager` and the `NoteSkins/` tree.
