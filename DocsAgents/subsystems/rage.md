---
type: Subsystem
title: Rage engine layer
description: The Rage* low-level layer — file I/O, display, sound pipeline, math, threads, timers, logging.
tags: [rage, engine, rendering, audio, io]
resource: src/
---

# Purpose

"Rage" is StepMania's in-tree engine layer: everything named `Rage*` in
`src/`. It abstracts the OS and provides files, graphics, sound, math,
threading and logging to the rest of the game. Ground-truth file grouping:
`src/CMakeData-rage.cmake` (groups: Utils, Misc, Graphics, File, Sound).

# Key files

| Area | Files | Notes |
|---|---|---|
| Files / VFS | `RageFileManager.*`, `RageFileDriver*` | Virtual filesystem with mount points; drivers for Direct, Zip, Deflate, Memory, Slice, ReadAhead, Timeout. `FILEMAN` global. |
| Display | `RageDisplay.*`, `RageDisplay_OGL*`, `RageDisplay_D3D*` (Win), `RageDisplay_GLES2*` (Linux), `RageDisplay_Null` | `DISPLAY` global. Backend chosen at startup from prefs. |
| Textures | `RageTexture*`, `RageBitmapTexture.*`, `RageSurface*` | `TEXTUREMAN` global. `RageSurface_Load_*` per image format. |
| Sound | `RageSound.*`, `RageSoundManager.*`, `RageSoundReader_*` | `SOUNDMAN` global. Readers form a filter chain (see below). |
| Misc | `RageInput*`, `RageLog.*`, `RageMath.*`, `RageTypes.*`, `RageThreads.*`, `RageTimer.*`, `RageException.*` | `LOG`, `INPUTMAN` globals. |
| Utils | `RageUtil*` | String/format/math helpers used everywhere. |

# Sound reader chain

`RageSoundReader_FileReader` opens the file; then wrappers are stacked as
needed: `Resample_Good`, `PitchChange`, `SpeedChange`, `Pan`, `Extend`,
`ChannelSplit`, `Merge`, `Chain`, `PreBuffering`/`PostBuffering`,
`ThreadedBuffer`, `Preload`. `RageSoundReader_MP3` / `_Vorbisfile` /
`_WAV` are the decoders. Mixing goes through `RageSoundMixBuffer`.

# Dependencies

- Down: `src/arch/` drivers, `extern/` libs (zlib, libpng, jpeg, ogg,
  vorbis, mad, glew).
- Up: used by actors, screens, data model — nearly everything.

# Gotchas

- **Never use `std::fstream` / raw `fopen`** for game assets — go through
  `RageFileManager` so VFS mounts, zips and memorycards work.
- Rage is not a library yet (`# TODO: Turn Rage into a library.` in the
  cmake file); there is no clean API boundary, includes are flat.
- `RageThreads` predates `std::thread`; match existing style when
  touching threaded code (`RageUtil_WorkerThread`, `BackgroundLoader`).
- `RString` (from `StdString.h`), not `std::string`, is pervasive here.
