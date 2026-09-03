---
type: Playbook
title: Build, run, and test StepMania-R
description: How to configure, compile, and test the engine on each platform.
tags: [build, cmake, ci, tests]
---

# Prerequisites

- **CMake** ≥ 2.8.12 (3.x recommended).
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
  [0003](./adr/0003-platform-support-floors.md)). The generated solution
  is `Build/StepMania.sln`.
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

- **Windows** — `cmake -B build` generates `Build/StepMania.sln` +
  `.vcxproj` files. Build in Visual Studio or `cmake --build build`. The
  `.exe` is placed in the repo root (`Program/`).
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

# Tests

Unit tests live in `src/tests/` (`test_*.cpp`): audio readers, file
errors/readers, misc, threads, timing data, vector. They are small
standalone programs, not a framework harness. See `src/tests/00 README`.
Check `src/CMakeLists.txt` / `src/CMakeProject-*.cmake` for whether a
given test target is wired into the current build.

CI additionally validates the Lua docs XML:
```
xmllint --noout Docs/Luadoc/Lua.xml
xmllint --noout Docs/Luadoc/LuaDocumentation.xml
```

# CI matrix

`.github/workflows/ci.yml` builds on Ubuntu x86_64, macOS arm64, macOS
x86_64, and Windows x86_64, plus the XML validation job. `paths-ignore`
skips CI for `**.md`-only changes — so knowledge-base edits under
`DocsAgents/` do not trigger a build.
