# tests/data/

Fixtures for `sm_tests`, mounted by `EngineTestEnv` at `/testdata`.

## Simfiles: use the real ones

The parse-regression (`test_NotesLoaderCorpus.cpp`, ADR 0006 phase 4)
does **not** use hand-authored toy simfiles. A toy `.sm`/`.ssc` only
proves the loader survives input the toy's author already understood; it
does not defend the `AGENTS.md` §5 invariant ("every simfile format the
engine loads today must keep loading, with identical results"). That
needs real charts with real quirks.

The regression loads the committed sample songs from the repo's `Songs/`
tree instead, via `EngineTestEnv::SongPath(...)`:

| Song | Formats | Notes |
|---|---|---|
| `Songs/StepMania 5/Goin' Under/` | `.sm` **and** `.ssc` | same song both ways -> cross-format equivalence check |
| `Songs/StepMania 5/MechaTribe Assault/` | `.ssc` | |
| `Songs/StepMania 5/Springtime/` | `.ssc` | |

`.sma` / `.dwi` / `.ksf` / `.crs` have no committed sample yet -- see
`DocsAgents/modernization-backlog.md` item 17. When the real song is
redistributable it goes under `Songs/`, not here.

## Derived simfile fixtures (when the real song is NOT redistributable)

`dwi-fixture/` -- a 3-chart `.dwi` for `test_NotesLoaderDWI.cpp`,
derived from a community simfile that isn't clearly redistributable. Only
`#FILE` / `#TITLE` / `#ARTIST` are changed from the source (verified by
diff); the note data, `#BPM`, `#GAP`, `#CHANGEBPM`, `#SAMPLESTART`,
`#RANDSTART` and all three `#SINGLE` blocks are byte-for-byte. DWI has no
keysounds, so there is nothing else to stub.

`pms-fixture/` -- a 3-chart `.pms` (Pop'n Music, BMS family) set for
`test_NotesLoaderBMS.cpp`. It is **derived** from a real song, not a copy
of one: the real Pop'n Music charts + keysound audio are Konami's and
cannot be committed to a public repo. So:

- Note data, timing, `#BPM` / `#BPMxx` changes, and the `#WAVxx`
  keysound-channel structure are kept **byte-for-byte** -- that is what
  `BMSLoader` actually parses.
- `#TITLE` / `#ARTIST` / `#GENRE` and every `#WAVxx` filename are
  replaced with generic placeholders.
- The keysound audio is replaced with 44-byte silent stub WAVs
  (`key<ID>.wav`), one per `#WAVxx` id. The loader only does `IsAFile`
  on them at parse time, so a silent stub is indistinguishable from the
  real sample for the parser.

This keeps the §5 regression honest (real chart/keysound *structure*)
without redistributing copyrighted content. Same approach applies to any
future `.bms` / `.pms` / `.dwi` fixture whose source is not free.

## What else lives here

Non-simfile fixtures: small binary/text inputs for the RageFile /
audio-reader round-trip tests when those are salvaged (backlog item 17).
