---
type: Architecture Decision
title: Platform support floors
description: Minimum supported OS versions — Windows 11, current macOS, current Linux — and what that lets us delete.
tags: [adr, platforms, windows, macos, linux]
---

# Status

**Accepted** — 2026-09-03. Maintainer decision. **Supersedes ADR 0001
§9** (which set a Windows 10 floor).

# Context

ADR 0001 §9 set the Windows floor at Windows 10 x64. The maintainer's
position: there is no reason for an actively-modernized independent
project (ADR 0002) to carry support code for operating systems people are
no longer running. Windows 10 reached end of support 2025-10; macOS and
Linux back-compat shims cost maintenance for users who don't exist.

# Decision

## Floors

| Platform | Minimum supported | Toolchain |
|---|---|---|
| **Windows** (P1) | **Windows 11 x64** | MSVC v143 / Visual Studio 2022 (or newer) |
| **macOS** (P2) | **latest released macOS and the one before it**; arm64 primary, x86_64 while Apple/Rosetta still ship it | current Xcode |
| **Linux** (P3) | **current mainstream distros** — target ~Ubuntu 24.04 LTS / current Fedora; glibc ≥ 2.38, GCC ≥ 13 or Clang ≥ 16 | current |

No 32-bit targets on any platform.

Platform **priority** is unchanged (`AGENTS.md` §3): Windows > macOS >
Linux; no macOS/Linux-specific work unless instructed.

## What this lets us delete (freely, as encountered)

- **Windows:** anything guarding Windows 7 / 8 / 8.1 / 10, XP-era
  (`v140_xp`, `_WIN32_IE 0x0400`), Win9x ("we support Win98 and WinME"),
  `_WIN32_WINNT` below `0x0A00`, `__STDC__ 0` and other VC2005/2008
  hacks, `_CRT_SECURE_NO_DEPRECATE`-style workarounds. Raise
  `_WIN32_WINNT` to `0x0A00` (there is no distinct Win11 value at that
  level) and use `NTDDI_VERSION` for anything Win11-specific.
- **macOS:** `MAC_OS_X_VERSION_MIN_REQUIRED` checks for < the current
  window, PowerPC remnants, Carbon, 32-bit `.mm` paths, deprecated
  10.x-only API fallbacks.
- **Linux:** pre-C++17 fallbacks, old-glibc shims, distro-specific hacks
  for EOL releases, build paths for compilers below the floor.

Each deletion is its own small commit (or a batched sweep of one area),
Windows build verified, pushed — same as any change (`AGENTS.md` §4).

## CI note

`.github/workflows/ci.yml` runs `windows-latest` (GitHub's image is still
~Windows Server 2022 / Win10 API level as of now). CI passing does **not**
prove the Win11 floor is exercised; that's fine — CI is a build check, the
floor is a support statement. Revisit runner images when GitHub ships
Server 2025.

# Consequences

- `_WIN32_WINNT` bump + VC2005/IE4 hack removal in
  `src/archutils/Win32/arch_setup.h` moves from "someday" to a near-term
  bounded change (backlog item 13).
- A new backlog item covers the broader sweep of pre-floor `#if` guards
  across `src/arch/` and `src/archutils/`.
- Bug reports from below-floor OSes are closed as out-of-scope.
- ADR 0001 §9 is now historical; this ADR is the reference for "what OS
  do we support".
