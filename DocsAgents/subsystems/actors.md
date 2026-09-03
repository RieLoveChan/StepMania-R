---
type: Subsystem
title: Actors (scene graph)
description: The Actor hierarchy — rendering primitives, tweening, and how themes instantiate them.
tags: [actors, rendering, scene-graph, tween]
resource: src/
---

# Purpose

Everything drawn on screen is an `Actor`. Screens own a tree of actors;
each frame the tree is updated (tweens advanced) then drawn. File
grouping: `src/CMakeData-actor.cmake` (Base, Gameplay, Menus, Gameplay
and Menus).

# Core types (`Actors/Base`)

| Class | Role |
|---|---|
| `Actor` | Base: position/rotation/zoom/color, tween queue, `Update`/`Draw`, command handling |
| `ActorFrame` | Container; propagates update/draw to children. Screens derive layout from frames |
| `ActorFrameTexture` | Render-to-texture frame |
| `Sprite` | Textured quad; animation states |
| `BitmapText` | Text via bitmap `Font` |
| `Model` / `ModelManager` / `ModelTypes` | 3D models (Milkshape) for characters |
| `Quad`, `ActorMultiVertex`, `ActorMultiTexture` | Raw geometry actors |
| `ActorScroller`, `DynamicActorScroller` | Scrolling item lists (used by wheels) |
| `ActorProxy` | Draws another actor again elsewhere |
| `ActorSound` | Plays a sound as part of the tree |
| `Tween` | Easing functions for the tween queue |
| `AutoActor` | Smart pointer that loads an actor from a theme path |

# How themes create actors

`ActorUtil` (`src/ActorUtil.*`) is the factory. `ActorUtil::MakeActor` /
`LoadFromNode` reads a theme node (Lua table or XML) and builds the actor,
dispatching on a `Class` / `Type` field. New actor types must
`REGISTER_ACTOR_CLASS( MyActor )` and expose a `LunaMyActor` binding
(see [lua.md](./lua.md)).

# Commands

Actors run **commands** (`src/Command.*`) — named sequences of
`property,value` pairs, historically from metrics, now mostly Lua
closures. `Actor::PlayCommand("On")` etc. `ActorUtil` wires standard
commands (`InitCommand`, `OnCommand`, `OffCommand`).

# Gotchas

- Draw order = tree order; use `ActorFrame` z / draw order, not manual
  sorting.
- Tweens are a queue; `stoptweening`/`finishtweening` matter. `sleep` and
  `queuecommand` are common building blocks.
- `Sprite` texture paths go through `RageTextureManager` / theme paths —
  don't load textures directly.
- Actor lifetime is owned by its parent `ActorFrame`; `ActorProxy` and
  `AutoActor` have subtle ownership rules.
- Gameplay actors (`NoteField`, `NoteDisplay`, life meters, score
  displays) are in this group but are documented under
  [gameplay.md](./gameplay.md).
