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

`.dwi` / `.ksf` have derived fixtures (below); `.sma` has a *synthetic*
one (below -- no real `.sma` exists anywhere); `.crs` has no sample yet.
See `DocsAgents/modernization-backlog.md` item 17. When a real
redistributable song turns up it goes under `Songs/`, not here.

## Derived simfile fixtures (when the real song is NOT redistributable)

`Fixture Artist - KSF Fixture/` -- a 4-chart KSF (Pump It Up format) set
for `test_NotesLoaderKSF.cpp`, derived from a copyrighted Pump It Up
song. Only `#TITLE` / `#ARTIST` / `#STEPMAKER` / `#SONGFILE` are changed
(diff-verified); `#BPM`, `#TICKCOUNT`, `#STARTTIME`, `#DIFFICULTY` and
every `#STEP` block are byte-for-byte. KSF has no keysounds. Two naming
constraints KSFLoader imposes are preserved: the filename drives type +
difficulty ("double" in the name -> pump_double + Medium; no keyword ->
pump_single + Hard), and the artist comes from the *directory* name
split on " - " (KSFLoader ignores `#ARTIST`), hence the dir name here.
Verified: identical chart output to the untouched source folder.

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

## Synthetic simfile fixtures (when NO real file exists at all)

`sma-fixture/` -- a 1-chart `.sma` for `test_NotesLoaderSMA.cpp`. Unlike
the derived fixtures above, this one is **invented from scratch**: the
`.sma` format is an extinct 2009-2011 SMA-editor variant and no real
file could be found anywhere. Per the maintainer's 2026-09-10 decision,
`SMALoader`'s current read behavior is taken as correct and this file
exists only to pin it against regression of the shared `SMLoader` base.
It deliberately exercises the SMA-only tags (`#ROWSPERBEAT`,
`#BEATSPERMEASURE`, `#SPEED` with the `s` seconds suffix, `#MULTIPLIER`)
-- see the header comment in the test for the pinned semantics. Every
value in it is made up; there is no source to diff against.

## What else lives here

Non-simfile fixtures: small binary/text inputs for the RageFile /
audio-reader round-trip tests when those are salvaged (backlog item 17).
