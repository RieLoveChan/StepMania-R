---
type: Architecture Decision
title: Threading modernization
description: Whether and how to replace RageThreads' pre-C++11 threading primitives with std::thread/mutex/condition_variable.
tags: [adr, threading, ragethreads, proposed]
---

# Status

**Proposed** — backlog item 11, explicitly flagged there as needing
"a dedicated ADR-scoped effort, not a casual pass." Not started.

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

# Open questions

**A. Pursue now, or stay deferred?** Nothing about the rest of the
backlog blocks on this — it's flagged because it's real debt, not
because anything is waiting on it.

**B. If pursued, what's the scope?**

1. **`RageThreads` internals only** — keep its existing class API
   (`RageMutex`, `RageEvent`, `RageThread`, etc.) exactly as call sites
   already use it, and reimplement the internals with
   `std::mutex`/`std::condition_variable`/`std::thread` under the
   hood. Lowest call-site churn, most contained risk.
2. **Also `RageUtil_AutoPtr.h`** — replace with `std::unique_ptr`/
   `std::shared_ptr` at every call site (the TODO already names this
   as the intent). Wider call-site churn, more mechanical.
3. **Also `RageUtil_WorkerThread`/`BackgroundLoader`** — re-derive their
   threading logic against the modernized primitives from (1) rather
   than just recompiling against a swapped-out `RageThreads`.

**C. Verification approach.** Given this session's own limits (Windows-
only local build, no real concurrent-load stress test available),
what would count as "done" here — CI's existing cross-platform build
+ the existing test suite passing, or does this need a dedicated
concurrency stress test added first (a new characterization test that
actually spins up multiple threads and checks for races/deadlocks,
which `src/tests/` doesn't have today)?

# Consequences (of pursuing it)

`RageThreads` sits underneath audio playback and background loading —
both perf- and correctness-sensitive, both hard to characterize with
the existing Catch2 harness (ADR 0006) as it stands today. Whatever is
decided in (C) likely becomes a precondition, not a nice-to-have.
