# Architecture Decision Records

Short records of decisions that shape the fork: the context, the choice,
and the consequences. An ADR is immutable once `Accepted` — supersede it
with a new one rather than editing.

Status values: `Proposed` (awaiting maintainer sign-off) · `Accepted` ·
`Superseded by NNNN` · `Deprecated`.

# Records

* [0001](./0001-toolchain-target.md) - Toolchain & modernization target (C++17, CMake 3.20, Win10/v143 floor, `-Werror` policy, FFmpeg binaries). **Status: Accepted.**
* [0002](./0002-independent-project.md) - Independent project; no upstream compatibility constraint. **Status: Accepted.**
* 0003 (not written) - Renderer strategy: legacy GL cleanup vs GL 3.3 core vs abstraction layer (bgfx et al.); fate of D3D9 and GLES2. Deferred here from ADR 0001.

# Adding one

Copy the structure of an existing record: `Status`, `Context`,
`Decision`, `Consequences`, `Open questions` (if Proposed). Number
sequentially, zero-padded to 4 digits. Log it in
[`../log.md`](../log.md).
