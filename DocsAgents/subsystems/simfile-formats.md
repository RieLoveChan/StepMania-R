---
type: Subsystem
title: Simfile loaders and writers
description: Parsers/writers for .sm .ssc .dwi .bms .ksf and JSON, plus the song cache.
tags: [simfile, loaders, writers, sm, ssc, formats]
resource: src/
---

# Purpose

Read simfiles from disk into `Song`/`Steps`/`TimingData`
([data-structures.md](./data-structures.md)) and write them back. File
grouping: `src/CMakeData-data.cmake` groups "Notes Loaders" / "Notes
Writers".

> **Hard invariant — `AGENTS.md` §5.** Every format here must keep
> loading, with no parse regression, forever. This is the one area where
> "independent project" (ADR 0002) does **not** grant freedom to break
> things — the ~20-year song library depends on it. Any change to a
> loader/writer/`MsdFile`/`TimingData` path is a §4 large change and
> needs corpus regression testing first.

# Loaders

| File | Format | Notes |
|---|---|---|
| `NotesLoader.*` | dispatcher | Picks a loader by extension; `NotesLoaderXXX::LoadFromDir` |
| `NotesLoaderSM.*` | `.sm` | Legacy StepMania format |
| `NotesLoaderSSC.*` | `.ssc` | Current format; split timing, more tags |
| `NotesLoaderSMA.*` | `.sma` | StepMania AMX variant |
| `NotesLoaderDWI.*` | `.dwi` | Dance With Intensity |
| `NotesLoaderBMS.*` | `.bms` | Be-Music Source (keysounded) |
| `NotesLoaderKSF.*` | `.ksf` | Pump It Up (Kick Style File) |
| `NotesLoaderJson.*` | `.json` | JSON chart |
| `LyricsLoader.*` | `.lrc` | Lyric timing |

# Writers

`NotesWriterSM.*`, `NotesWriterSSC.*`, `NotesWriterDWI.*`,
`NotesWriterJson.*`. The editor and "save" paths use SSC.

# Format references (`Docs/`, editable — ADR 0002)

- `Docs/SimfileFormats/` — DWI, BMS, KSF, SDF, dance-spec
- `Docs/Userdocs/sm5_beginner_simfiles.txt`, `Docs/Themerdocs/`
- `Docs/Devdocs/SplitTiming.txt`, `WarpNotes.txt`, `NegBPMsTutorial.html`
- `Docs/Changelog_SSCformat.txt` — SSC tag history

# Gotchas

- **`.ssc` is the source of truth going forward.** New tags → add to
  `NotesLoaderSSC` + `NotesWriterSSC` + bump the cache version in
  `SongCacheIndex` (or old caches serve stale data).
- `#OFFSET` sign conventions and BPM/stop parsing differ subtly between
  formats — copy an existing tag's handling, don't reinvent.
- BMS/KSF bring keysounds and non-4-column styles; they exercise
  `Style` and `AutoKeysounds` paths the others don't.
- Loaders must be tolerant of malformed community files — fail soft, log,
  keep loading other songs.
- Parsing helpers: `MsdFile.*` (`#TAG:value;` MSD parser) underlies
  SM/SSC/DWI.
- There are loader tests: `src/tests/test_file_readers.cpp`,
  `test_timing_data.cpp`.
