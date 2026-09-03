---
type: Architecture Decision
title: Independent project — no upstream compatibility
description: StepMania-R is its own project; keeping merges with stepmania/stepmania clean is no longer a constraint.
tags: [adr, fork, upstream, scope]
---

# Status

**Accepted** — 2026-09-02. Maintainer decision.

# Context

StepMania-R began as a fork of `stepmania/stepmania` (`5_1-new`). Until
now the working rules (`AGENTS.md` §2, ADR 0001) treated
"stay mergeable with upstream / keep the merge surface minimal" as a hard
constraint. In practice that constraint:

- forbade repo-wide changes (mass refactor, repo-wide `clang-format`, a
  `RString` → `std::string` campaign) purely to avoid a large diff;
- made `Docs/` and other upstream-authored files off-limits;
- forced code/knowledge commit separation for merge cleanliness;
- capped the pace and depth of modernization.

Upstream has been effectively inactive for ~11 years and is not a source
of changes worth merging. The constraint costs more than it returns.

# Decision

**StepMania-R is an independent project.** Compatibility with
`stepmania/stepmania` is not a goal. Breaking it — API, file layout,
formats, build, whatever — is acceptable and expected when it serves the
project.

Concretely:

- **No obligation to keep merges with upstream clean.** Do not scope,
  split, or avoid a change because it would be a large diff *versus
  upstream*. (Diff size still matters for **review** — see Consequences.)
- **Repo-wide changes are permitted**: mass mechanical refactors, a
  one-shot repo-wide `clang-format`, a full `RString` migration, deleting
  dead platform code wholesale.
- **`Docs/` is now editable project documentation.** Fix it, restructure
  it, delete what is obsolete. Agent-facing knowledge still lives in
  `DocsAgents/` (separation of audience, not hygiene).
- **Commit separation (code vs knowledge) is optional good practice**, no
  longer a MUST.
- No PR-template obligation; nothing to "raise upstream".

# What does NOT change

- **Platform priority** (Windows > macOS > Linux) — `AGENTS.md` §3.
- **The §4 process stays** — but as of 2026-09-03 it is
  *commit-and-push autonomously; maintainer reviews async and reverts*,
  **not** a blocking manual-verification gate. The safety mechanism is
  cheap reverts + small single-purpose commits, not a stop.
- **`DocsAgents/` remains the knowledge base**; the deposit rule
  (`AGENTS.md` §7.1) still applies.
- **Simfile / community-content compatibility is a hard invariant**
  (`AGENTS.md` §5). ADR 0002 frees us from the *original project*, not
  from the ~20-year library of user-made songs. Every supported simfile
  format keeps loading, with no parse regression.
- **Incremental, per-subsystem work remains the default** — but now as a
  *consequence of having no safety net and limited review capacity*, not
  as an upstream constraint. Once a real test harness + headless smoke
  test exist ("step 2"), the bar for a bigger single change drops.

# Consequences

- `AGENTS.md` §2 is rewritten from "Fork hygiene rules" to plain
  repository conventions.
- ADR 0001 items whose rationale was "unmergeable vs upstream" are
  reworded to stand on safety/review grounds only (Settled #4, #5).
- `modernization-backlog.md` item 6: just fix `Build/README.md`.
- Playbooks lose the "would conflict with upstream" cautions; the
  underlying advice (keep diffs reviewable, don't mix a reformat into a
  logic change) stays, justified by review cost and `git blame` noise.
- Review capacity is now the main limiter on change size, backed by the
  §4 gate. Bigger changes are allowed; they are not automatically safe.
