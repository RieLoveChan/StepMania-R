# AGENTS.md — StepMania-R

Entry point for any coding agent working in this repository. Read this
fully, then start from [`DocsAgents/index.md`](./DocsAgents/index.md).

---

## The point of all this: de-hard-code the engine

Every specific task in this project is an instance of one goal:
**StepMania bakes into C++ things that should be data, configuration, or
an extensible registry.** Twenty years of "just put it in the `.cpp`" —
game types, driver lists, layout numbers, platform assumptions, code that
assumes an old on-disk layout. The direction of every change is to move
those *out of the compiler's reach*: into something a person can edit
without rebuilding, or that the engine resolves at run time.

**Recognise hard-coding.** Signs a thing should become data:

- A compile-time `enum` or `static const` array that enumerates
  **content** — game types + the `StepsType` enum, note types, difficulty
  names. Adding one means editing a huge `.cpp` and rebuilding.
- Magic numbers for on-screen layout / timing windows / colours that live
  in code instead of theme metrics (`THEME->GetMetric*`).
- A hand-maintained list of backends / drivers / handlers dispatched by
  `if( name == "..." )` chains.
- Fixed paths, buffer sizes, OS version constants, `#ifdef` ladders for
  specific platforms.
- Code that assumes a legacy on-disk shape and silently "fixes it up"
  (the `Char Widths` / `IniFile::RenameKey` case).

**"De-hard-code" resolves to — pick the fit:** a theme metric; a data
file loaded at startup (Lua or an `.ini` tree — `NoteSkins/` and
`Themes/` already work this way); a runtime registry keyed by string/id
instead of a compile-time enum; a CMake or `Preferences.ini` option; a
plugin / extension point.

**Guardrails — do NOT over-apply:**

- This is a **direction and a tiebreaker, not** "stop and rip out every
  constant". De-hard-code a thing when you are already in that file for
  another reason, or as a deliberately scoped change.
- **On-disk contracts are stable contracts, not hard-coding to remove.**
  `#STEPSTYPE` strings, `.ssc` tag names, simfile formats,
  `FILE_CACHE_VERSION` semantics — see §5. Making the *engine*
  data-driven must keep every existing on-disk value resolving
  identically; never "flexibilise" a file format itself.
- Hot paths, security-relevant code, and genuine invariants stay in code.
- Same process as everything else (§4): incremental, Release-build
  verified, small commits. A large one (e.g. `StepsType` enum → runtime
  id) gets its own ADR.

**Applying it:** when you meet a hard-coded thing, ask *"should a person
be able to change this without recompiling?"* If yes and it is small —
do it as part of the change in hand. If yes and it is large — add a
[`DocsAgents/modernization-backlog.md`](./DocsAgents/modernization-backlog.md)
item (items 10, 18, 20 and ADR 0004 are existing examples) and, if it is
architectural, propose an ADR.

---

## 1. Language rules (MUST)

- **Reply to the user in the same language they wrote to you in.** If they
  switch languages, you switch with them.
- **Everything written into the repository MUST be in English.** This is
  non-negotiable and applies to:
  - Source code identifiers, comments, and log strings.
  - Commit messages and PR descriptions.
  - Every `.md` file under `DocsAgents/` and any other documentation.
  - Test names and fixtures.

  The user-facing conversation may be in any language; the artifacts you
  commit may not.

## 2. Repository conventions

StepMania-R is an **independent project**. Compatibility with
`stepmania/stepmania` is not a goal — breaking it is acceptable (ADR
[0002](./DocsAgents/adr/0002-independent-project.md)). There is no
"upstream merge" to protect, so nothing here is about minimizing a diff
against upstream. It is about keeping the repo legible.

- **Agent-facing knowledge goes under `DocsAgents/`** (the OKF bundle).
  `Docs/` is human-facing project documentation and *is* editable now —
  but keep the two audiences separate: don't put subsystem notes /
  playbooks in `Docs/`, and don't turn `DocsAgents/` into end-user docs.
- **Keep agent config centralized:** `AGENTS.md` (this file) and the
  git-ignored repo-root `CLAUDE.md` are the only agent files outside
  `DocsAgents/`. Prefer not to scatter `CLAUDE.md` files into `src/`
  subtrees — put subsystem knowledge in `DocsAgents/subsystems/`.
- **Prefer separate commits for code vs knowledge-base changes** — good
  hygiene and keeps history readable, but no longer mandatory.
- When you change code under `src/<area>/`, check whether the matching
  `DocsAgents/subsystems/*.md` concept needs an update (see §7.1).
- **Diff size still matters — for review, not for merges.** A large
  mechanical change (repo-wide reformat, mass refactor) is *allowed*
  (ADR 0002) but must go through the §4 verification gate and should not
  be mixed into a logic change.

## 3. Platform priority & support floors (MUST)

1. **Windows is the primary target.** Every change must build and run on
   Windows. This is the platform the maintainer develops and verifies on.
2. **macOS is second**, **Linux is third** — kept building (CI covers
   them) but not actively developed.
2a. **Support floors (ADR
   [0003](./DocsAgents/adr/0003-platform-support-floors.md)):** Windows
   11 x64; latest macOS + the one before; current Linux distros. Support
   code for anything older (Win7/8/10, XP/9x hacks, `_WIN32_WINNT` <
   `0x0A00`, old-macOS `.mm` fallbacks, EOL-distro shims, 32-bit) may be
   **deleted on sight** — small commit, Windows build verified, pushed.
3. **Do not do macOS- or Linux-specific work unless explicitly
   instructed.** That includes touching `src/arch/*/`*`_MacOSX`* /
   *`_Unix`* / *`_X11`* files, macOS/Linux build tooling, or `.mm` files.
   If a cross-platform change happens to require a matching tweak in a
   non-Windows driver to keep CI green, keep it minimal and call it out
   explicitly in the PR description.

## 4. Change process (MUST)

Modernization is **continuous and autonomous. Do not wait for maintainer
approval** — commit and push. Worst case is a `git revert`, which is
cheap by design (ADR [0002](./DocsAgents/adr/0002-independent-project.md)).

Per change:

1. Make the change, keeping it **small and single-purpose** so a revert
   is surgical. Code and knowledge-base edits in separate commits (§2).
2. **Confirm the Windows Release build is green** (`--config Release` —
   that is `Program/StepMania-R.exe`, what ships and what the maintainer
   plays; `-config Debug` = `StepMania-R_debug.exe` differs: `/Od`, asserts
   on, `DEBUG` macro, `/RTC1`). Optimisation can surface warnings/UB that
   Debug hides. Then the `--SelfTest` smoke
   (`Program/StepMania-R.exe --SelfTest --VideoRenderers=null
   --SoundDrivers=null` → exit 0). Do not push a change you know is broken.
3. Commit with a message that says **what to spot-check** (the affected
   screen / feature / path) so the maintainer's async review is quick.
4. `git push` to `origin 5_1-new`. Continue to the next change.

The maintainer reviews after the fact and reverts anything wrong. That is
the safety mechanism — not a blocking gate.

**Higher-risk changes** — public-interface or observable-behavior
changes, cross-file type/pattern migrations (`RString` → `std::string`),
toolchain/dependency/build-flag bumps, anything touching the simfile
parse path (§5), or roughly >1 dozen files — still get committed and
pushed, but: split them as finely as possible, spell out the spot-check
in the message, and prefer a short-lived branch + note to the maintainer
if a single revert could not cleanly undo it.

## 5. Simfile & content compatibility (MUST)

ADR 0002 permits breaking compatibility with the original project. It
does **not** permit breaking compatibility with the ~20-year library of
community content. This is a hard invariant:

- **Every simfile format the engine loads today must keep loading**, with
  identical results — notes, timing, metadata, radar values. Formats in
  scope: `.sm`, `.ssc`, `.sma`, `.dwi`, `.bms`/`.bml`/`.pms`, `.ksf`,
  plus `.crs` courses and `.lrc` lyrics. Dropping a loader is forbidden.
- **No regression in parse behavior.** A song that loads correctly now
  must load byte-for-byte equivalently after your change. Fixing a
  genuine parser bug is allowed only with the maintainer's explicit
  sign-off and a test that pins old vs new.
- **`.ssc` is the canonical write format.** New `#TAG`s are additive and
  fine (unknown tags are tolerated on load). You may **not** change the
  meaning of an existing tag, remove one, or emit output that a prior
  StepMania 5.x `.ssc` reader would choke on.
- **The song cache is internal** — change it freely (bump
  `FILE_CACHE_VERSION`). The on-disk simfile is the source of truth and
  its parsing may not regress.
- **Any change touching the parse/write path** (`NotesLoader*`,
  `NotesWriter*`, `MsdFile`, `TimingData`, `NoteData`, `Song`, `Steps`
  loading) is a §4 large change **and** must be regression-tested against
  a simfile corpus before the manual gate.

See [`DocsAgents/subsystems/simfile-formats.md`](./DocsAgents/subsystems/simfile-formats.md)
and [`DocsAgents/playbooks/add-ssc-tag.md`](./DocsAgents/playbooks/add-ssc-tag.md).

## 6. How the knowledge base is organized

`DocsAgents/` is an **Open Knowledge Format (OKF) bundle** — a tree of
Markdown files with YAML frontmatter. The full spec is in
[`DocsAgents/spec.md`](./DocsAgents/spec.md). Practical rules for consumers:

- Start at [`DocsAgents/index.md`](./DocsAgents/index.md). It is the
  routing table for the whole repo: read it first, then open only the one
  or two concept documents relevant to your task. Do not load the whole
  bundle.
- Each concept has frontmatter with a `type` field (`Subsystem`,
  `Playbook`, `Reference`, `Architecture Decision`, `Gotcha`). Use it to
  decide relevance before reading the body.
- Cross-links are bundle-relative (`/DocsAgents/...`) or relative
  (`./...`). A broken link means "not written yet", not an error.
- [`DocsAgents/log.md`](./DocsAgents/log.md) is the chronological history
  of knowledge-base changes. Append to it when you add or materially
  change a concept.

## 7. How to author into the bundle

When you learn something durable that the next agent would need:

1. Pick or create the right concept file (subsystem doc, playbook, gotcha).
2. Add frontmatter: at minimum `type:`, plus `title:` and `description:`.
3. Prefer structure (headings, tables, lists, fenced code) over prose.
4. Add a dated entry to [`DocsAgents/log.md`](./DocsAgents/log.md).
5. Keep it in English (§1).

Do **not** duplicate what the code, `git log`, or `Docs/` already say.
Capture the non-obvious: architecture, invariants, "why", gotchas, and
where to look.

### 7.1 Deposit rule (MUST)

The knowledge base is only worth its upkeep if it stops the *next* agent
from re-solving what you just solved. So:

- **Before** starting a task, read the matching
  [`DocsAgents/playbooks/`](./DocsAgents/playbooks/index.md) entry and the
  relevant [`subsystems/`](./DocsAgents/subsystems/index.md) concept. Do
  not rediscover what is already written.
- **After** a task that needed more than one correction, or that hit a
  non-obvious dead end, leave a trace so the next attempt takes fewer
  steps:
  - repeatable procedure → add/fix a **playbook** (use
    `playbooks/_template.md`);
  - one-off specific → a `# Gotchas` bullet in the subsystem concept;
  - a decision → an **ADR** under `DocsAgents/adr/`.
- **Findings from the maintainer's manual verification (§4) go in the
  same way** — the reviewer's correction is exactly the knowledge the
  next agent is missing.

Prefer few well-scoped changes over rapid churn. A task done right in a
few steps beats one done fast through many corrections.
