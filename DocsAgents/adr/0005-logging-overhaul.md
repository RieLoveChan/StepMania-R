---
type: Architecture Decision
title: Logging overhaul
description: Make RageLog output scannable and filterable — bracketed levels, no ///// frames, categories, repeat-collapsing.
tags: [adr, logging, ragelog, debugging]
---

# Status

**Accepted** — 2026-09-03. Maintainer decision. Phase 1 landing with this
ADR.

# Context

`RageLog` (`src/RageLog.{h,cpp}`) is 2004-era:

- Levels are `Trace` / `Info` / `Warn` only — no `Error`, no `Debug`, no
  runtime threshold.
- `Warn()` frames **every** line with
  `/////////////////////////////////////////` before and after (a
  scrolling-console hack). One warning = 3 lines.
- No category / subsystem tag. The codebase fakes it by hand
  (`LOG->Info("[Class::Func] ...")` per `CodingStyle.txt`).
- No `file:line`. No repeat de-duplication.

Measured on a `--SelfTest` boot (695 lines of `log.txt`):

| | lines | % |
|---|---:|---:|
| `/////` frame lines | 152 | 22% |
| `WARNING: Key 'Char Widths' not found.` (benign) | 76 | 11% |
| `glTexImage2D` traces | 68 | 10% |
| `Load script … .lua` | 95 | 14% |

The **only** distinct WARN in the whole boot is a false one
(`Char Widths`, see below). Real problems would be invisible.

# Decision

Rework `RageLog` in phases. Format model:

```
00:03.069  [WARN]   font   Font.cpp:474  Char Widths section absent; using [main]
00:03.055  [TRACE]                        Loading screen "ScreenSystemLayer"
```

- **Every line carries a bracketed level tag**: `[TRACE] [DEBUG] [INFO]
  [WARN] [ERROR]`, fixed width for column alignment. Greppable:
  `grep '\[WARN\]'`, `grep -E '\[(WARN|ERROR)\]'`.
- **No `/////` frames.** The `[WARN]` / `[ERROR]` tag is the marker.
- Levels: `Trace < Debug < Info < Warn < Error`. `Error` = serious but
  recoverable (below `RageException::Throw`).
- Later: per-category threshold (`--LogLevel=gl:off,font:trace`),
  `file:line` via macro / `std::source_location`, repeat-collapsing
  (`… (repeated 76×)`), tighter dev-log vs `userlog.txt` split.

## Phases

1. **Format** (this ADR): rewrite `RageLog::Write` — bracketed padded
   level tag, drop the frame, drop the `"WARNING: "` string prefix. Add
   `RageLog::Error()`. No call-site changes. + fix `Char Widths` (below).
2. `Debug` level + per-category thresholds + `--LogLevel` flag +
   `Log::Category` enum and a `LOG_*` macro layer that captures
   `file:line`.
3. Repeat-collapsing in `AddToRecentLogs` / `Write`.
4. **Call-site audit** (long tail, per subsystem, like the tidy passes):
   `LOG->Warn` that are really `Trace` (expected fallback) → demote;
   genuine failures → `LOG->Error`; add category tags.

## The `Char Widths` bug (fixed in Phase 1)

`Font.cpp:474,798` call `ini.RenameKey("Char Widths", "main")` for
backward-compat with pre-`[main]` font `.ini`s. `IniFile::RenameKey`
(`IniFile.cpp:217`) does `LOG->Warn("Key '%s' not found.")` when the
source key is absent — which it always is for modern fonts. A
rename-if-present is not a warnable event. Fix: `RenameKey` logs a
`Trace`, not a `Warn`, on a missing source key. Drops ~76 warn lines +
~152 frame lines from every boot.

# Consequences

- Phase 1 changes the shape of every log line — trivial to eyeball, and
  `--SelfTest` output drops from 695 to ~470 lines with real signal
  visible.
- External tools / muscle memory that grep `WARNING:` need to grep
  `\[WARN\]` instead. Acceptable (independent project, ADR 0002).
- Phases 2-4 are separate commits; 4 is opportunistic per subsystem.
