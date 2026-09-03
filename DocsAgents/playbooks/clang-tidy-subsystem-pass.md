---
type: Playbook
title: Run a modernization pass over one subsystem
description: Apply a bounded set of clang-tidy / warning fixes to a single subsystem, verify the Windows build, commit and push.
tags: [modernization, clang-tidy, warnings, process]
---

# Goal

Move one subsystem measurably closer to modern C++ / zero-warnings, in a
single reviewable PR, without behavior change.

# When to use

The core loop of the modernization effort (see
[../index.md](../index.md) → "Why this fork exists"). One subsystem
([../subsystems/index.md](../subsystems/index.md)), one bounded check set,
one PR.

# Prerequisites (already set up as of 2026-09-03)

| Thing | Where |
|---|---|
| `.clang-tidy` (starter check set) | repo root — additive; widen per pass |
| `clang-tidy.exe` + `ninja.exe` | `scratchpad/tools/` (LLVM 23.1.0); not on PATH by default |
| `build-tidy/compile_commands.json` | Ninja tree; regen recipe in [`../baseline.md`](../baseline.md) → "How to (re)generate" (needs the hand-built MSVC/SDK env — `vcvars64.bat` is broken here) |
| Baseline counts | [`../baseline.md`](../baseline.md) → clang-tidy section |

Run clang-tidy as: `clang-tidy.exe -p build-tidy --quiet --extra-arg-before=/Y- <file>`
(`/Y-` disables the never-built PCH — required, else every file errors).

# Files touched

Only files under the chosen subsystem's `src/...` glob, plus:

| Path | Change |
|---|---|
| `DocsAgents/baseline.md` | Update the count for this subsystem |
| `DocsAgents/subsystems/<name>.md` | Add a `# Gotchas` note if the pass surfaced one |
| `DocsAgents/log.md` | Dated entry |

# Steps

1. **Pick the scope:** one subsystem, and one narrow check family, e.g.
   `modernize-use-nullptr`, `modernize-use-override`,
   `modernize-loop-convert`, `readability-redundant-*`,
   `modernize-use-equals-default`. Do **not** run "all modernize-*" over a
   big subsystem in one go.
2. **Measure before:** run clang-tidy over the subsystem's files, record
   the warning count and the check breakdown.
3. **Apply:** `clang-tidy -p build --fix --checks='-*,modernize-use-nullptr' <files>`
   (or apply by hand for anything the fixer does clumsily).
4. **Review every hunk.** clang-tidy fixers are not always behavior-safe
   (macros, template context, `NULL` used as int, `auto` changing type).
   Revert anything questionable.
5. **Build Windows** (primary target) — must compile clean. Build macOS/
   Linux only if CI complains; do not chase non-Windows-only fixes
   (`AGENTS.md` §3).
6. **Measure after**, update `../baseline.md`.
7. **Commit split:** code changes in one commit, `DocsAgents/` updates in
   a separate commit (`AGENTS.md` §2).

# Gotchas

- **Not behavior-preserving by default.** `modernize-use-auto` can change
  deduced types; `modernize-use-emplace` can change overload resolution;
  `use-nullptr` touching a variadic call is a real bug. Review, don't
  rubber-stamp the `--fix` output.
- The build already sets `-Wall -Wextra` (GCC/Clang) / `/W4` (MSVC) with
  suppressions in `src/CMakeLists.txt:126-132`. Reducing real warnings is
  in scope; **removing the suppressions** is a separate, deliberate PR.
- When a warning category reaches **zero across `src/`**, promote it to
  `-Werror=<cat>` under the `WITH_WERROR` option (ADR 0001 §7) so it
  cannot regress. Record it in [`../baseline.md`](../baseline.md).
- `RString` is not `std::string` — `modernize-*` string checks may
  misfire on it. Full `RString` migration is its own playbook, not this.
- Keep each PR small enough that manual verification is cheap — that is
  the point of scoping to one check family.
- Do not reformat untouched lines in this PR. A repo-wide `clang-format`
  is *allowed* (ADR 0002) but must be its own dedicated change — mixing
  it into a tidy pass makes both unreviewable and wrecks `git blame`.

# Verification

- Windows build green, no new warnings, headless smoke test passes (once
  it exists).
- `../baseline.md` number for this subsystem/check is lower; update it.
- **One commit per (subsystem × check family)** — small enough that a
  single `git revert` cleanly undoes the pass. Message names the check,
  the subsystem, and the before/after count. Then `git push`
  (`AGENTS.md` §4 — no approval wait).

# History

- 2026-09-02 — created. Warning flags at `src/CMakeLists.txt:126-132`;
  C++17 / CMake 3.20 already in effect.
- 2026-09-03 — `.clang-tidy` (starter set) and `baseline.md` now exist.
  Repo-wide baseline: `container-size-empty` 463, `use-override` 367,
  `macro-parentheses` 197 lead (all autofix). Tooling recipe (Ninja
  `build-tidy`, manual MSVC/SDK env, `--extra-arg-before=/Y-`) in
  `baseline.md` → "How to (re)generate".
