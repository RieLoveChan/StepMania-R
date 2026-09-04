---
type: Playbook
title: Build, run, and test StepMania-R
description: How to configure, compile, and test the engine on each platform.
tags: [build, cmake, ci, tests]
---

# Prerequisites

- **CMake** ≥ 3.20 (`CMakeLists.txt:1`). Language standard is C++17
  (`src/CMakeLists.txt:88`).
- **Submodules** — the `extern/` libraries are git submodules. Always:
  ```
  git submodule update --init --recursive
  ```
- **nasm** — required on all platforms (FFmpeg / asm codecs).
- Linux: dev headers for ALSA, GL/GLU, GTK3, JACK, libmad, PulseAudio,
  udev, usb, Xinerama, X11, Xrandr, Xtst (see `.github/workflows/ci.yml`
  for the exact `apt-get` list).
- Windows: **Visual Studio 2022 (MSVC v143)** or newer, x64. Minimum
  supported runtime target is **Windows 11 x64** (ADR
  [0003](./adr/0003-platform-support-floors.md)). `cmake -B build`
  generates `build/StepMania.sln`.
- macOS: current Xcode; latest macOS and the one before it; arm64
  primary. Linux: current mainstream distros (~Ubuntu 24.04 / current
  Fedora). See ADR [0003](./adr/0003-platform-support-floors.md).
- macOS: Xcode; nasm via `brew install nasm`.

# Configure + build (all platforms)

CI does exactly this (`.github/workflows/ci.yml`):

```
cmake -B build
cmake --build build
```

Platform notes:

- **Windows** — `cmake -B build` generates `build/StepMania.sln` +
  `.vcxproj` files. Build in Visual Studio or `cmake --build build`. The
  `.exe` is placed in the repo root (`Program/StepMania-R.exe`).
  For a **Ninja or raw command-line** build (no Visual Studio generator),
  a plain `vcvars64.bat` leaves the Windows SDK unwired
  (`WindowsSdkDir` empty) — MSBuild papers over this via `.vcxproj`
  properties, but `cl.exe`/`link.exe`/Ninja have no such fallback and
  the compiler-detection step in `cmake -G Ninja -B build` fails.
  Dot-source `Build/dev-env.ps1` first to fix that (backlog item 14):
  ```powershell
  . .\Build\dev-env.ps1
  cmake -G Ninja -B build
  ```
- **macOS** — pass `-DCMAKE_OSX_ARCHITECTURES=arm64` (or `x86_64`).
  Produces `StepMania.app`.
- **Linux** — `cmake -B build && cmake --build build` (or plain `make`
  after `cmake .`). Produces `stepmania` + optional `GtkModule.so` in the
  repo root.

The binary runs from the **repo root** — it expects `Themes/`,
`NoteSkins/`, `Songs/`, `Data/` beside it. There is no separate install
step needed for development.

# Common CMake options

Defined in `CMake/DefineOptions.cmake`. Pass as `-DWITH_X=ON/OFF`:

| Option | Default | Purpose |
|---|---|---|
| `WITH_FULL_RELEASE` | OFF | Proper release build |
| `WITH_LTO` | OFF | Link-time optimization |
| `WITH_SSE2` | ON | SSE2 codegen |
| `WITH_CRASH_HANDLER` | ON | Built-in crash reporter |
| `WITH_CLUB_FANTASTIC` | OFF | Bundle Club Fantastic songs |
| `WITH_GLES2` / `WITH_GTK3` (Linux) | ON | GL ES 2.0 / GTK3 UI |
| `WITH_ALSA` / `WITH_PULSEAUDIO` / `WITH_JACK` (Linux) | ON/ON/OFF | Audio backends |
| `WITH_WERROR` | OFF | Treat the engine's own warnings as errors (CI) |
| `WITH_TESTS` | OFF | Build `sm_tests` (Catch2); makes `src/` an OBJECT library |

# Tests

Two layers:

- **Headless smoke** — `Program/StepMania-R.exe --SelfTest
  --VideoRenderers=null --SoundDrivers=null` runs full engine init and
  exits 0. This is the `AGENTS.md` §4 smoke gate; it runs in Windows CI.
- **Unit / characterization** — `WITH_TESTS=ON` builds `src/` as the
  `sm_engine` OBJECT library and adds the `sm_tests` target (Catch2 v3,
  vendored at `extern/Catch2/`). Sources in `tests/`.

  ```
  cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug -DWITH_TESTS=ON
  cmake --build build-tests --config Debug --target sm_tests
  ctest --test-dir build-tests -C Debug --output-on-failure
  ```

  `WITH_TESTS=OFF` (the default, and every build except the dedicated CI
  job) leaves `src/CMakeLists.txt` behaving exactly as before the split.
  See ADR [0006](./adr/0006-test-harness.md) and
  [`playbooks/add-characterization-test.md`](./playbooks/add-characterization-test.md).

The old `src/tests/` `test_*.cpp` (2004-era, Unix-only, uncommitted data)
are **not** wired and are being replaced file-by-file under `tests/`.

CI additionally validates the Lua docs XML:
```
xmllint --noout Docs/Luadoc/Lua.xml
xmllint --noout Docs/Luadoc/LuaDocumentation.xml
```

# CI matrix

`.github/workflows/ci.yml` builds on Ubuntu x86_64, macOS arm64, macOS
x86_64, and Windows x86_64, plus the XML validation job, the Windows
headless smoke, and `windows-tests` (`-DWITH_TESTS=ON` build + `ctest`).
`paths-ignore` skips CI for `**.md`-only changes — so knowledge-base
edits under `DocsAgents/` do not trigger a build.
