---
type: Architecture Decision
title: Threading modernization
description: Whether and how to replace RageThreads' pre-C++11 threading primitives with std::thread/mutex/condition_variable.
tags: [adr, threading, ragethreads, proposed]
---

# Status

**Accepted** — 2026-09-15. Maintainer decision. Backlog item 11.

# Decision

- **Scope: `RageThreads` internals only.** Keep its existing class API
  (`RageMutex`, `RageEvent`, `RageThread`, etc.) exactly as call sites
  already use it; reimplement the internals with
  `std::mutex`/`std::condition_variable`/`std::thread` under the hood.
  `RageUtil_AutoPtr.h`, `RageUtil_WorkerThread`, and `BackgroundLoader`
  are **not** in scope for this pass.
- **Verification precondition: a new concurrency stress test comes
  first.** Before touching `RageThreads` itself, add a dedicated
  Catch2 test that spins up multiple real threads against the current
  implementation and checks for races/deadlocks — nothing in
  `src/tests/` does this today (ADR 0006's harness is otherwise
  single-threaded). That test must pass against the *old*
  implementation first (proving it's a real, working check, not a
  no-op), then again against the reimplemented internals.

# Context

`RageThreads` (`src/RageThreads.{h,cpp}`) predates `std::thread` (C++11)
and implements its own cross-platform mutex/thread/event-wait wrappers
over the native Win32 and pthreads APIs. Related pre-C++11 idioms in
the same neighborhood:

- `RageUtil_AutoPtr.h` — carries its own `// TODO: replace with c++11
  smart pointers` comment already in the code.
- `RageUtil_WorkerThread` — a worker-thread base class built on
  `RageThreads`.
- `BackgroundLoader` — background asset loading, also threaded.

This code is **load-bearing and cross-platform**: it underlies the
audio callback thread, background loading, and (per item 15) a
complete-but-unwired mutex lock-order deadlock-detection feature
already sitting in `RageThreads.cpp`. A bug here is a race condition
or deadlock — not a compile error — and this session's tooling can
only verify a Windows build; real cross-platform threading behavior
needs the actual CI runners (or a maintainer's own machine) exercising
real concurrency, which a green build does not prove.

# Consequences

`RageThreads` sits underneath audio playback and background loading —
both perf- and correctness-sensitive, both hard to characterize with
the existing Catch2 harness (ADR 0006) as it stands today. Whatever is
decided in (C) likely becomes a precondition, not a nice-to-have.
