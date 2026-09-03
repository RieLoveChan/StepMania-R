---
type: Playbook
title: Add a new .ssc simfile tag
description: Wire a new #TAG through the SSC loader, writer, and the song cache.
tags: [simfile, ssc, loader, writer, cache]
---

# Goal

Add a new `#MYTAG:value;` field to the `.ssc` format so it round-trips:
parsed on load, written on save, and not stale-served from the cache.

# When to use

Adding song-level or steps-level metadata/timing to `.ssc`. Not for
`.sm`/`.dwi`/`.bms`/`.ksf` (those have their own loaders and are legacy —
usually read-only). If the value is timing, it also belongs in
[TimingData](../subsystems/data-structures.md).

# Files always touched

| Path | Change |
|---|---|
| `src/NotesLoaderSSC.cpp` | Add a `Set*` handler func + register it in the handler map |
| `src/NotesWriterSSC.cpp` | Emit the `#TAG:...;` line in the corresponding write section |
| `src/Song.h` / `src/Song.cpp` **or** `src/Steps.h` / `src/TimingData.*` | The field that stores the value |
| `src/Song.cpp` (`FILE_CACHE_VERSION`) | **Bump it** — see Gotchas |
| `DocsAgents/subsystems/simfile-formats.md` | Note the new tag if non-obvious |

# Steps

1. **Decide scope:** song tag or steps tag. Song tags live under
   `SongTagInfo` handlers; steps tags under `StepsTagInfo` handlers
   (`src/NotesLoaderSSC.cpp`, structs near line 26–70).
2. **Add storage** for the value on `Song` / `Steps` / `TimingData`.
3. **Loader:** write `static void SetMyTag(SongTagInfo& info)` (or
   `StepsTagInfo&`) next to the other `Set*` funcs. Read from
   `info.params[1]` (the MSD value). Register it in the map builder:
   `song_tag_handlers["MYTAG"]= &SetMyTag;` (around line 541+) or
   `steps_tag_handlers["MYTAG"]= &SetMyTag;` (around line 605+).
   Tag names are **UPPERCASE**, matched exactly.
4. **Writer:** in `src/NotesWriterSSC.cpp`, add
   `f.PutLine(ssprintf("#MYTAG:%s;", ...));` in the matching block
   (song header ~line 216+, timing block ~line 185+, or the per-Steps
   block). Use `SmEscape()` for free-text string values.
5. **Bump `FILE_CACHE_VERSION`** in `src/Song.cpp` (currently `227`).
6. Update `Lua.xml` / bindings only if the field is Lua-exposed
   (separate task — see [expose-lua-api](./expose-lua-api.md)).

# Gotchas

- **Cache staleness.** Parsed songs are cached in an SQLite DB
  (`SongCacheIndex`, `Cache/`). If you do not bump `FILE_CACHE_VERSION`,
  existing installs keep serving the old parse and your tag silently does
  nothing. This is the #1 thing forgotten here.
- Loader and writer tag lists drift apart easily — add to **both** in the
  same commit, and check the value survives a load→save→load cycle.
- `STEPFILE_VERSION_NUMBER` (`src/Song.h`, `0.83`) is the on-disk
  `#VERSION` written into files; only touch it for a real format-level
  version change, not per-tag.
- Legacy loaders (`NotesLoaderSM` etc.) will not know the tag; that is
  expected. Don't try to back-port unless asked.
- Unknown tags are tolerated by the loader (logged, skipped) — so a
  missing loader handler fails soft, which makes the bug easy to miss.
- Record the tag in `Docs/Changelog_SSCformat.txt` (that file is editable
  now — ADR 0002) and in the PR description.

# Verification

- Windows build green (+ `--SelfTest` headless smoke).
- **Corpus regression test the parse path** (`AGENTS.md` §5) before
  pushing — load→save→load a set of real simfiles, diff the parsed
  state. The simfile invariant is not just "review it later".
- Add a case to `src/tests/test_timing_data.cpp` (or a new loader test)
  if the tag is timing-related.
- Then commit + push. Spot-check to note: create a tiny `.ssc` with the
  tag, load/save/reload, confirm it survives and takes effect. Anything
  touching `TimingData` semantics is higher-risk — split finely.

# History

- 2026-09-02 — created from code recon (`NotesLoaderSSC.cpp` handler
  maps, `FILE_CACHE_VERSION=227`, `NotesWriterSSC.cpp` layout). Steps not
  yet executed end-to-end; verify the line anchors before relying on them.
