---
type: Subsystem
title: NoteSkins
description: NoteSkinManager and the NoteSkins/ tree — how a note's appearance is resolved.
tags: [noteskins, rendering, gameplay]
resource: NoteSkins/
---

# Purpose

A NoteSkin defines how notes, receptors and hold bodies look and animate
for a given `StepsType`. `NoteSkinManager` (`NOTESKIN`) loads them;
`NoteDisplay` ([gameplay.md](./gameplay.md)) draws with the active one.

# Layout

```
NoteSkins/
  <game>/            e.g. dance, pump, popn  (note: NoteSkins/popn/ is currently untracked in this fork)
    <skinname>/
      metrics.ini        per-skin config (colors, blend, animation)
      *.lua / *.png       element actors + textures
      Fallback.ini        which skin to fall back to
  common/            shared elements
```

# Key files

| File | Role |
|---|---|
| `src/NoteSkinManager.*` | `NOTESKIN` global; enumerate skins, resolve element paths, per-skin metrics |
| `src/NoteDisplay.*` | Consumes the skin to draw taps/holds/rolls/mines per column |
| `src/ReceptorArrow*.*`, `src/GhostArrowRow.*` | Receptor + hit-flash actors from the skin |
| `Docs/Themerdocs/Noteskin elements Reference.txt` | Element list (do not edit) |

# Resolution

`NOTESKIN->GetPath( sButton, sElement )` searches: current skin → its
`Fallback.ini` chain → `common`. Elements are keyed by column/button name
(from the `Style`) and element (`Tap Note`, `Hold Body Active`, `Receptor`,
`Tap Mine`, …).

# Gotchas

- Skins are **per game type**; a `dance` skin has 4/8 columns, `pump` has
  5, `popn` 9 — element sets differ.
- `NoteSkins/popn/` is present but **untracked** (shows in `git status`).
  Decide deliberately whether to commit it.
- Note-color-by-beat (4th/8th/12th/16th…) is a skin concern via
  `NoteDisplay` + skin metrics, tied to `NoteTypes.*`.
- Performance: `NoteDisplay` runs per visible note per frame; skin Lua
  should be cheap.
- Adding an element type touches `NoteDisplay`, the skin `metrics.ini`
  schema, and the Themerdocs reference.
