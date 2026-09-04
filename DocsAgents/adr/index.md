# Architecture Decision Records

Short records of decisions that shape the fork: the context, the choice,
and the consequences. An ADR is immutable once `Accepted` — supersede it
with a new one rather than editing.

Status values: `Proposed` (awaiting maintainer sign-off) · `Accepted` ·
`Superseded by NNNN` · `Deprecated`.

# Records

* [0001](./0001-toolchain-target.md) - Toolchain & modernization target (C++17, CMake 3.20, `-Werror` policy, FFmpeg binaries). **Status: Accepted.** (§9 superseded by 0003.)
* [0002](./0002-independent-project.md) - Independent project; no upstream compatibility constraint. **Status: Accepted.**
* [0003](./0003-platform-support-floors.md) - Platform support floors: Windows 11, current macOS, current Linux; supersedes 0001 §9. **Status: Accepted.**
* 0004 (not written) - Renderer strategy: legacy GL cleanup vs GL 3.3 core vs abstraction layer (bgfx et al.); fate of D3D9 and GLES2. Deferred from ADR 0001.
* [0005](./0005-logging-overhaul.md) - Logging overhaul: bracketed level tags, no `/////` frames, categories, repeat-collapsing. **Status: Accepted** (phase 1 landed).
* [0006](./0006-test-harness.md) - Test harness: Catch2 v3 (amalgamated) + `src/` as an OBJECT library so exe and `tests/` share one engine build. **Status: Accepted** (scaffold on branch, merge gated on green build).

# Adding one

Copy the structure of an existing record: `Status`, `Context`,
`Decision`, `Consequences`, `Open questions` (if Proposed). Number
sequentially, zero-padded to 4 digits. Log it in
[`../log.md`](../log.md).
