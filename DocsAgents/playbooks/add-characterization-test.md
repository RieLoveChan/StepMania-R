---
type: Playbook
title: Add a characterization test
description: Pin the current behaviour of an engine function with a Catch2 test under tests/, before refactoring it.
tags: [testing, catch2, safety-net, characterization]
---

# Goal

Add a test that locks in what an engine function does **right now**
(bug-for-bug), so a later refactor of that area fails loudly if it
changes an observable result.

# When to use

- Before touching a "pure-ish" core (`RageUtil`, `RageMath`,
  `TimingData`, `NoteData`/`NoteDataUtil`, `NotesLoader*`) — build the net
  first, then refactor.
- When a maintainer review turns up a behaviour worth freezing.

Not for: code that needs a live `GAMESTATE` / renderer / audio device
(that is smoke-test territory, `--SelfTest`). Not for asserting what the
code *should* do — a characterization test records what it *does*; fix
bugs in a separate, labelled change with the maintainer's sign-off
(`AGENTS.md` §5 for anything on the simfile path).

# Files always touched

| Path | What changes |
|---|---|
| `tests/test_<Area>.cpp` | new `TEST_CASE`s; new file → also add to `tests/CMakeLists.txt` |
| `tests/CMakeLists.txt` | add the new `.cpp` to `SM_TEST_SRC` |
| `DocsAgents/baseline.md` | bump the "Tests" note if coverage meaningfully grows |

# Steps

1. **Read the implementation** in `src/` before writing expectations —
   `awk '/^RString Foo\(/{f=1} f{print; if(/^}/)exit}' src/RageUtil.cpp`.
   Predict outputs from the code, not from the function name (see the
   `Capitalize` / `GetExtension` quirks already pinned in
   `test_RageUtil.cpp`).
2. Add `TEST_CASE("<what it does>", "[<area>][<facet>]")` to
   `tests/test_<Area>.cpp`. Include order: `#include "global.h"` →
   engine headers → `#include "catch_amalgamated.hpp"` last.
3. New file: add it to `SM_TEST_SRC` in `tests/CMakeLists.txt`.
4. Configure + build + run:
   ```
   cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug -DWITH_TESTS=ON
   cmake --build build-tests --config Debug --target sm_tests
   ctest --test-dir build-tests -C Debug --output-on-failure
   ```
5. If an assertion surprises you, re-check the implementation. The test
   encodes reality; only "correct" it if reality changed.

# Gotchas

- **`WITH_TESTS=ON` switches `src/` to the `sm_engine` OBJECT library.**
  Use a *fresh* build dir (`build-tests/`), not the normal `build/`.
- The test binary links the **whole engine** — a link error usually means
  the function's translation unit pulls a symbol nothing else in
  `sm_tests` provides; it is a coupling signal, not a test bug. Widen the
  test or note the coupling; do not start hand-picking `src/*.cpp`.
- `RString` is `StdString::CStdString` — comparisons against string
  literals work; prefer `== "literal"` over constructing an `RString`.
- Catch2's `main` comes from the vendored `catch_amalgamated.cpp` (the
  `Catch2` lib). Do **not** define your own `main` in a test file.
- MSVC: the engine objects are `/SAFESEH`-less and (Debug) link the debug
  CRT — `tests/CMakeLists.txt` already mirrors those link flags for
  `sm_tests`; keep them if you edit that file.

# Verification

- Green: `ctest --test-dir build-tests -C Debug` (all `sm_tests` cases
  pass) **and** a normal `-DWITH_TESTS=OFF` Release build still links
  (the OBJECT-library split must stay transparent when off).
- Maintainer spot-check: the new `TEST_CASE` names + that the pinned
  values match a manual read of the function. Low-risk once the harness
  itself is merged — a new test file is additive.

# History

- `2026-09-03` — created alongside ADR
  [0006](../adr/0006-test-harness.md); first file `test_RageUtil.cpp`.
