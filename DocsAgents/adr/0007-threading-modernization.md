---
type: Architecture Decision
title: Threading modernization
description: Whether and how to replace RageThreads' pre-C++11 threading primitives with std::thread/mutex/condition_variable.
tags: [adr, threading, ragethreads, proposed]
---

# Status

**Accepted** — 2026-09-15. Maintainer decision. Backlog item 11.

**Implemented and closed — 2026-09-16.** `MutexImpl`/`EventImpl`/
`SemaImpl` (the `RageThreads.h` internals this ADR covers) are now one
portable `std::mutex`/`std::condition_variable`/`std::condition_variable_any`-backed
implementation (`src/arch/Threads/Threads_Std.{h,cpp}`), replacing the
separate `MutexImpl_Win32`/`EventImpl_Win32`/`SemaImpl_Win32` and
`MutexImpl_Pthreads`/`EventImpl_Pthreads`/`SemaImpl_Pthreads` classes
(767 lines net removed from `Threads_Win32.*`/`Threads_Pthreads.*`,
which now only keep `ThreadImpl` -- thread creation/Halt/Resume/naming
-- since `std::thread` has no portable suspend and `Halt()` is a real
crash-handler safety feature, not something to weaken for uniformity).

The precondition was met, with two findings along the way (see
`tests/test_RageThreadsConcurrency.cpp` for full detail):
- The stress test, run against the *old* implementation first, caught a
  genuine intermittent bug in `SemaImpl_Win32`: its diagnostic
  `m_iCounter` was mutated by `Post()`/`Wait()`/`TryWait()` with no lock
  protecting it, an unsynchronized read-modify-write race across
  threads. `SemaImpl_Std` fixes this (every access goes through the same
  `std::mutex`).
- A separate bug turned up in the test itself, not either
  implementation: a mutex may only be unlocked by the thread that
  locked it. `std::mutex` enforces this strictly (aborts via the debug
  CRT); the old Win32/pthreads primitives tolerated it more loosely,
  which is exactly the kind of latent-bug risk this ADR exists to
  reduce.

Verified: `sm_tests` (235 cases including the new stress test, run
repeatedly) and `--SelfTest` both clean on Windows, plus the real engine
run past `--SelfTest` into its normal main loop (audio mixer/decode/
input threads all real `RageThread`s) for 15s with no threading errors.
Non-Windows behavior (the `Threads_Pthreads.cpp` split, and
`Threads_Std.cpp` itself, are platform-agnostic C++ with no Windows-only
code) relies on CI (Ubuntu/macOS build + unit-test jobs) for
verification, per this ADR's own Context section.

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
