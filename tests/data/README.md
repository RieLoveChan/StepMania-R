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

Other formats (`.sma`, `.dwi`, `.ksf`, `.bms`, `.crs`) have no committed
sample yet -- see `DocsAgents/modernization-backlog.md` item 17. When one
is added it goes under `Songs/`, not here.

## What does live here

Non-simfile fixtures: small binary/text inputs for the RageFile /
audio-reader round-trip tests when those are salvaged (backlog item 17).
