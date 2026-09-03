---
type: Subsystem
title: Content data model
description: Song, Steps, NoteData, TimingData, Style, Course, Profile, HighScore and the scoring types.
tags: [data-model, song, steps, notedata, timing, scoring]
resource: src/
---

# Purpose

The in-memory representation of songs, charts, timing, styles, player
progress and scores. File grouping: `src/CMakeData-data.cmake`
(Songs, Steps and Styles, Note Data, Score Keepers, Courses, Fonts, Lua,
Misc Objects).

# Core objects

| Type | Files | Notes |
|---|---|---|
| `Song` | `Song.*`, `SongUtil.*`, `SongOptions.*`, `SongPosition.*` | One simfile folder. Holds metadata, `TimingData`, a list of `Steps`, background changes. |
| `Steps` | `Steps.*`, `StepsUtil.*` | One chart (difficulty × stepstype). Lazily decompresses `NoteData`. |
| `NoteData` | `NoteData.*`, `NoteDataUtil.*`, `NoteDataWithScoring.*` | Column×row grid of `TapNote`. `NoteDataUtil` = transforms (mirror, mods, stats, radar). |
| `TimingData` | `TimingData.*`, `TimingSegments.*` | BPM/stop/warp/delay/scroll/etc. segments; beat↔second conversion. **Central to sync.** |
| `Style` / `StepsType` | `Style.*`, `StyleUtil.*` | Maps game inputs to chart columns (e.g. `dance-single` = 4 cols). Defined by `GameManager`. |
| `Course` / `Trail` | `Course.*`, `Trail*.*`, `CourseUtil.*` | Course mode: ordered song list → generated `Trail`. |
| `Profile` | `Profile.*` | Per-player persistent data; serialized via `NotesWriter`-style XML/DB by `ProfileManager`. |
| `HighScore` | `HighScore.*` | A single score record. |
| `ScoreKeeper` | `ScoreKeeper*.*` | Live scoring during play: `Normal`, `Rave`, `Shared`. See [gameplay.md](./gameplay.md). |
| `PlayerOptions` / `PlayerState` / `PlayerStageStats` | `PlayerOptions.*` etc. | Per-player mods, live state, per-song results. |
| `RadarValues` | `RadarValues.*` | The "groove radar" chart stats. |

# Loading / cache

`Song` objects are built by the loaders in
[simfile-formats.md](./simfile-formats.md). `SongCacheIndex`
(`SongCacheIndex.*`, global `SONGINDEX`) caches parsed songs in an SQLite
DB under `Cache/` for fast startup.

# Gotchas

- **`TimingData` is the tricky one.** Negative BPMs, warps, and
  delays vs stops have subtle semantics — see `Docs/Devdocs/WarpNotes.txt`,
  `NegBPMsTutorial.html`, `SplitTiming.txt`. Split timing = per-`Steps`
  timing overriding song timing.
- `NoteData` is stored compressed on `Steps`; call
  `Steps::GetNoteData()` / `Decompress()` — don't poke raw members.
- Adding a chart property usually touches: the loader(s), the writer(s),
  `Steps`/`Song`/`TimingData`, the cache version bump, and often Lua.
- Radar/stat values are computed in `NoteDataUtil` — keep them in sync if
  you add a note type (`NoteTypes.*`).
- `SongUtil`/`StepsUtil` hold the sorting/filtering predicates used by the
  music wheel.
