---
type: Reference
title: Coding conventions and project rules
description: Code style for this C++ codebase plus project contribution rules.
tags: [style, conventions]
---

# Project rules

See [`AGENTS.md`](../AGENTS.md) §1–§5 and ADR
[0002](./adr/0002-independent-project.md) (authoritative). Summary:

- Conversation language follows the user; **committed artifacts are English only**.
- StepMania-R is an **independent project** — no upstream compatibility
  to preserve.
- **Simfile compatibility is a hard invariant** (`AGENTS.md` §5): every
  supported format (`.sm` `.ssc` `.sma` `.dwi` `.bms` `.ksf` `.crs`
  `.lrc`) keeps loading with no parse regression. Non-negotiable.
- Agent-facing knowledge goes under `DocsAgents/`; `Docs/` is editable
  human-facing documentation.
- Prefer separate commits for code vs knowledge-base changes (not required).
- **Windows is the primary target**; no macOS/Linux-specific work unless
  explicitly instructed.
- **Commit and push autonomously — do not wait for approval** (AGENTS.md
  §4). Confirm the Windows build is green, keep commits small and
  single-purpose, say what to spot-check in the message. The maintainer
  reviews async and reverts anything wrong.

# C++ style (from `Docs/Devdocs/CodingStyle.txt`)

- **Tabs for indentation** (author assumes width 4). Spaces allowed for
  alignment only. Enforced-ish by `.editorconfig` and `.clang-format`.
- Brace style: Allman is the modern preference —
  ```cpp
  if( someCondition )
  {
      doThing();
  }
  ```
  Older one-line `if( c )\n\tfoo();` still exists; match the file you edit.
- Spaces inside parens for clarity: `if( x )`, `foo( a, b )`.
- **Naming:** largely Hungarian. `m_` prefix for class members
  (`m_pFoo`, `m_iCount`, `m_bEnabled`, `m_sName`, `m_fValue`). Newer code
  sometimes drops Hungarian prefixes — follow local context.
- **Logging:** prefix with class/function —
  `LOG->Info( "[NetworkSyncManager::Listen] ..." )`. Log levels via the
  `RageLog` global `LOG` (`Trace`/`Info`/`Warn`).
- Comments: `//` for one-liners and short blocks; `/* ... */` for long
  form with the opening text on the first line. Don't reformat copyright
  blocks.
- Remove trailing whitespace.

# Tooling in the repo

| File | Purpose |
|---|---|
| `.clang-format` | Formatter config (C++) |
| `.editorconfig` | Tab/charset rules for editors |
| `Docs/Devdocs/CodingStyle.txt` | Full prose style guide |
| `Docs/Devdocs/GoldenRules.txt` | Project doctrine (social, not code) |
| `.github/PULL_REQUEST_TEMPLATE.md` | PR checklist (inherited; revise for this project as needed) |

# Idioms specific to StepMania

- **Global singletons** are accessed through ALL-CAPS pointers declared in
  each manager header: `SCREENMAN`, `SONGMAN`, `GAMESTATE`, `THEME`,
  `PREFSMAN`, `INPUTMAPPER`, `SOUND`, `NOTESKIN`, `PROFILEMAN`, etc. See
  [subsystems/singletons.md](./subsystems/singletons.md).
- **Lua exposure:** a class `Foo` gets a `LunaFoo` binding class and
  `ADD_METHOD` calls, usually at the bottom of `Foo.cpp`. See
  [subsystems/lua.md](./subsystems/lua.md).
- **Themable values** are read via `THEME->GetMetric*( sClass, sName )`
  and `THEME->GetPathTo*`. Hard-coded UI numbers are discouraged.
- **RString** — legacy string type (`StdString.h`); much code still uses
  it instead of `std::string`. `RageUtil.h` has the string helpers.
- `global.h` is the precompiled-header-ish common include; most `.cpp`
  files include it first.
