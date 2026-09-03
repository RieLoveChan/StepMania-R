---
type: Playbook
title: Migrate one subsystem from RString to std::string
description: Retire RString in a bounded scope without a repo-wide diff, keeping boundaries with un-migrated code intact.
tags: [modernization, rstring, strings]
---

# Goal

Remove `RString` from one subsystem's `.cpp`/`.h`, replacing it with
`std::string`, while every call site that crosses into not-yet-migrated
code still compiles and behaves identically.

# When to use

Per ADR 0001 (Settled #5): `RString` retirement is **opportunistic and
per-subsystem**, never a global campaign. Do this when you are already
working in a subsystem for another reason, or as a dedicated small PR for
a leaf subsystem with few external string interfaces.

# Background

- `RString` is `typedef StdString::CStdString RString;` (`src/global.h:107`).
  `CStdString` is `CStdStr<char>`, a subclass of `std::basic_string<char>`
  — so it *is* a `std::string` plus extra methods.
- Scale: ~723 files, ~8,429 uses. This is why it cannot be one diff.
- Extra API that `std::string` lacks (the porting surface):
  `.Format(...)`, `.MakeUpper()` / `.MakeLower()`, `.Left(n)` /
  `.Right(n)` / `.Mid(...)`, `.Trim*()`, `.SpanExcluding(...)`, plus some
  implicit `const char*` conveniences.

# Files touched

Only the chosen subsystem's files, plus its `DocsAgents/subsystems/*.md`
if a boundary gotcha turned up, plus `log.md`.

# Steps

1. **Scope tight.** One subsystem (`DocsAgents/subsystems/` boundary).
   Prefer one with few functions that take/return strings across the
   boundary. Count them first: `rg -n "RString" <files>`.
2. **Map the extra API to `RageUtil` free functions** (they already
   exist for non-`RString` use):
   | `RString` method | Replacement |
   |---|---|
   | `s.Format("%d", x)` | `s = ssprintf("%d", x)` |
   | `s.MakeUpper()` / `MakeLower()` | `MakeUpper(s)` / `MakeLower(s)` (RageUtil) |
   | `s.Left(n)` | `s.substr(0, n)` |
   | `s.Right(n)` | `s.substr(s.size()-n)` (guard `n <= size`) |
   | `s.Mid(i, n)` | `s.substr(i, n)` |
   | `s.Trim()` | `Trim(s)` (RageUtil) |
   | `s.SpanExcluding(set)` | `RageUtil` equivalent / manual |
3. **Replace `RString` → `std::string`** in the subsystem's own decls,
   locals, members, and internal signatures.
4. **At the boundary:** functions called from un-migrated code — keep the
   signature accepting/returning something both sides accept. Since
   `CStdString` derives from `std::string`, a function taking
   `const std::string&` still accepts an `RString` argument from a caller.
   Returning `std::string` where callers expect `RString` is usually fine
   (implicit construct). Verify each boundary function compiles from the
   caller side.
5. **Do not touch** `global.h`, `StdString.h`, or the `RString` typedef.
   It stays until the last subsystem is done (a final ADR retires it).
6. **Build Windows**, review every hunk, keep formatting local.

# Gotchas

- **`printf`-family + `std::string`:** `ssprintf("%s", s.c_str())` — a
  bare `std::string` into a varargs `%s` is UB. `RString` sometimes
  papered over this; do not carry the bug.
- `RString` had case-insensitive-ish helpers and `operator==` with
  `const char*`; `std::string` comparisons are exact. Watch equality
  checks that relied on `CStdString` conveniences.
- Serialization / `IniFile` / `XmlFile` / MSD parsing APIs are heavy
  `RString` users — a subsystem that talks to them has a big boundary;
  pick a different subsystem first.
- clang-tidy `modernize-*` string checks may misfire while `RString` is
  still in the translation unit — run tidy *after* the migration, not to
  drive it.
- Lua bindings (`SArg`, `lua_pushstring`) take `const char*` / build
  `RString` — check the `Luna*` block if the subsystem has one.

# Verification

- Windows build green, no new warnings, `--SelfTest` smoke green, then
  commit + push (`AGENTS.md` §4).
- Higher-risk (cross-file type migration): commit **per subsystem**, one
  subsystem per commit, so a revert is a single clean undo. Spell out in
  the message what to spot-check — string-heavy code is easy to break
  with subtle formatting/trim differences.

# History

- 2026-09-02 — created from recon: `global.h:107`, `StdString.h` API
  surface, usage counts. No subsystem migrated yet — first executor
  fills in a "known-good order" list of subsystems here.
