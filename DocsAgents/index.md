---
okf_version: "0.1"
---

# StepMania-R — Agent Knowledge Base

Routing table for agents working in this repository. Read this page, then
open only the concept(s) relevant to your task.

**What this is:** an **independent project** that started as a fork of
[StepMania](https://github.com/stepmania/stepmania) (`5_1-new`), an
advanced cross-platform rhythm game. C++ engine (~780 files in `src/`),
Lua-scripted themes, CMake build.

**Why it exists:** the original project has gone ~11 years without
shipping a GitHub release (last one ~2015) while other rhythm / PMS/BMS
simulators caught up to modern standards. StepMania-R's goal is
**modernization and actually shipping releases**. Compatibility with the
original repo is **not** a goal — breaking it is fine (ADR
[0002](./adr/0002-independent-project.md)). "Does this move the codebase
toward a modern, releasable state" is a valid tiebreaker when
prioritizing.

**The technical north star:** *de-hard-code the engine* — move baked-in
C++ decisions (game types, driver lists, layout numbers, platform
assumptions) into data / config / runtime registries, without breaking
the on-disk contracts with the simfile library. Full statement:
[`AGENTS.md`](../AGENTS.md) → "The point of all this".

Before editing anything, read [`AGENTS.md`](../AGENTS.md) — the language
rules, platform priority, and verification gate are mandatory.

---

## Start here by task

| Your task involves… | Read |
|---|---|
| A recurring task with a known procedure | [`playbooks/`](./playbooks/index.md) — **check here first** |
| Modernization work (what/why/priority) | [`modernization-backlog.md`](./modernization-backlog.md), [`adr/0001`](./adr/0001-toolchain-target.md) |
| The measured starting state (warnings, tests, size) | [`baseline.md`](./baseline.md) |
| Building, compiling, running tests | [`build.md`](./build.md) |
| Coding style, conventions, fork rules | [`conventions.md`](./conventions.md) |
| Understanding overall flow / where a feature lives | [`architecture.md`](./architecture.md) |
| A specific engine area | [`subsystems/`](./subsystems/index.md) |
| A past decision and its rationale | [`adr/`](./adr/index.md) |
| History of this knowledge base | [`log.md`](./log.md) |
| The knowledge-format spec itself | [`spec.md`](./spec.md) |

After a task that took corrections, deposit what you learned back here —
see [`AGENTS.md`](../AGENTS.md) §7.1.

## Subsystems

See [`subsystems/index.md`](./subsystems/index.md) for the full list. Quick map:

| Concept | Covers | Code |
|---|---|---|
| [rage](./subsystems/rage.md) | Low-level engine layer: file I/O, OpenGL/D3D display, sound pipeline, threads, math, timers, input devices | `src/Rage*.{cpp,h}` |
| [arch](./subsystems/arch.md) | Per-OS driver implementations (Windows/macOS/Linux) selected at build time | `src/arch/`, `src/archutils/` |
| [actors](./subsystems/actors.md) | Scene-graph rendering primitives: `Actor`, `ActorFrame`, `Sprite`, `BitmapText`, `Model`, tweening | `src/Actor*.{cpp,h}`, `src/Sprite.*`, etc. |
| [screens](./subsystems/screens.md) | Screen state machine and all `Screen*` classes (gameplay, options, select music, edit, evaluation) | `src/Screen*.{cpp,h}` |
| [singletons](./subsystems/singletons.md) | Global `*Manager` objects and `GameState`; lifetime and access patterns | `src/*Manager.{cpp,h}`, `src/GameState.*` |
| [data-structures](./subsystems/data-structures.md) | `Song`, `Steps`, `NoteData`, `TimingData`, `Style`, `Course`, `Profile`, `HighScore`, score keepers | `src/CMakeData-data.cmake` group |
| [simfile-formats](./subsystems/simfile-formats.md) | Parsers/writers for `.sm` `.ssc` `.dwi` `.bms` `.ksf` and JSON; the cache | `src/NotesLoader*.{cpp,h}`, `src/NotesWriter*.{cpp,h}` |
| [gameplay](./subsystems/gameplay.md) | The play mechanic: `Player`, `NoteField`, `NoteDisplay`, `ArrowEffects`, judgment, life meters, scoring | `src/Player.*`, `src/NoteField*.*`, `src/ScoreKeeper*.*` |
| [lua](./subsystems/lua.md) | Lua 5.1 binding layer, `LuaManager`, `LuaReference`, how C++ classes are exposed to themes | `src/Lua*.{cpp,h}`, `src/LuaBinding.*` |
| [theming](./subsystems/theming.md) | `ThemeManager`, metrics (`metrics.ini`), `Themes/` data tree, actor templates, localization | `src/ThemeManager.*`, `Themes/` |
| [noteskins](./subsystems/noteskins.md) | `NoteSkinManager`, the `NoteSkins/` data tree, how a skin resolves | `src/NoteSkinManager.*`, `NoteSkins/` |
| [input](./subsystems/input.md) | Input path from device to game button: `RageInput` → `InputFilter` → `InputMapper` → `InputQueue` | `src/Input*.{cpp,h}`, `src/RageInput*.*` |

## Search recipes

The fastest index is `ripgrep`. Common patterns:

- Find a class: `rg -n "class ClassName" src/`
- Find a singleton's global: most managers expose a global pointer, e.g.
  `SCREENMAN`, `SONGMAN`, `THEME`, `GAMESTATE`, `PREFSMAN` — `rg -n "\bTHEME\b" src/`
- Find a Lua-exposed method: `rg -n "ADD_METHOD|LunaClassName" src/`
- Find a metric read: `rg -n "THEME->GetMetric" src/`
- Find where a screen is pushed: `rg -n "SCREENMAN->SetNewScreen|SystemMessage" src/`
- Source-group / subsystem membership: `src/CMakeData-*.cmake` list files
  are the project's own grouping of translation units.

## Inherited documentation (`Docs/`)

Human-facing docs carried over from the original project. Editable now
(ADR [0002](./adr/0002-independent-project.md)) — fix or restructure what
is stale — but keep agent-facing knowledge in `DocsAgents/`, not here.

| Path | Content |
|---|---|
| `Docs/Devdocs/CodingStyle.txt` | Canonical code style |
| `Docs/Devdocs/GoldenRules.txt` | Project doctrine |
| `Docs/Luadoc/Lua.xml` | Generated Lua API reference (validated in CI) |
| `Docs/Themerdocs/` | Theme authoring reference |
| `Docs/Userdocs/`, `Docs/SimfileFormats/` | Simfile format specs |
| `Docs/CommandLineArgs.txt` | Runtime CLI flags |
| `Build/INSTALL.md`, `Build/README.md` | Build instructions |
