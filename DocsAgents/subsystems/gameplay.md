---
type: Subsystem
title: Gameplay mechanic
description: Player, NoteField, NoteDisplay, ArrowEffects, judgment, life meters and live scoring.
tags: [gameplay, player, notefield, judgment, scoring]
resource: src/
---

# Purpose

The actual rhythm-game mechanic: scrolling notes, timing judgment, life,
and score. Orchestrated by `ScreenGameplay` ([screens.md](./screens.md)).
File grouping: `src/CMakeData-actor.cmake` "Actors/Gameplay" +
`src/CMakeData-data.cmake` "Score Keepers".

# Key classes

| Class | Files | Role |
|---|---|---|
| `Player` | `Player.*` | Per-player brain: reads input, judges taps/holds against `NoteData` + `TimingData`, updates stats, drives feedback. `PlayerPlus`/`PlayerAI` variants. |
| `NoteField` | `NoteField.*` | The playfield actor: owns receptors + scrolling notes for one player. |
| `NoteDisplay` | `NoteDisplay.*` | Draws individual notes using the active NoteSkin; handles animation. |
| `ArrowEffects` | `ArrowEffects.*` | Math for note position given mods (speed, accel, wave, drunk, tornado, reverse…). Pure-ish functions. |
| `ReceptorArrow*`, `GhostArrowRow` | receptor + hit flash | |
| `HoldJudgment`, `Judgment` (theme) | hold/tap judgment feedback | |
| `LifeMeter*` | `LifeMeterBar/Battery/Time`, `CombinedLifeMeterTug` | Life/health models per play mode. |
| `ScoreKeeper*` | `ScoreKeeperNormal/Rave/Shared` | Converts judgments → score; grade thresholds. |
| `ScoreDisplay*` | many | Themed score readouts. |
| `GameplayAssist` | `GameplayAssist.*` | Assist tick / clap. |
| `AutoKeysounds` | `AutoKeysounds.*` | Plays BMS/keysound audio in time. |
| `PlayerStageStats` | results accumulator for evaluation | |

# Frame flow during play

`ScreenGameplay::Update` → `Player::Update` (advance song time via
`GAMESTATE`/`SongPosition`) → judge rows crossing the receptor →
`ScoreKeeper::HandleTapNoteScore` / life meter update → `NoteField` /
`NoteDisplay` draw using `ArrowEffects` positions and the NoteSkin.

# Gotchas

- **Timing windows** (`TapNoteScore` W1..W5, hold windows) come from
  **theme metrics** (`[Gameplay]`/`[Player]`) and prefs, not constants.
- `ArrowEffects` is performance-critical and mod math is order-sensitive;
  changing it affects every note every frame.
- Judgment must use the chart's `TimingData` (split timing!) not wall
  clock — see [data-structures.md](./data-structures.md).
- Multiplayer/`Rave`/`Battle`/`Oni` each have their own scorekeeper and
  life meter; a change to "scoring" usually means several files.
- Feedback actors are theme-driven; `Player` sends messages/commands, the
  theme decides visuals.
