# Building StepMania-R

StepMania-R is configured with **CMake**. This document covers configuring
the build; see [`INSTALL.md`](./INSTALL.md) for compiling and (optionally)
installing afterwards.

## Requirements

- **CMake ≥ 3.20.**
- A **C++17** compiler:
  - Windows: **Visual Studio 2022** (MSVC v143) or newer, x64.
  - macOS: current Xcode.
  - Linux: current GCC or Clang.
- **nasm** — required on every platform (FFmpeg / asm codecs).
- The `extern/` libraries are git submodules:

  ```
  git submodule update --init --recursive
  ```

- Linux only: development headers for ALSA, GL/GLU, GTK3, JACK, libmad,
  PulseAudio, udev, usb, Xinerama, X11, Xrandr, Xtst. See
  `.github/workflows/ci.yml` for the exact `apt-get` list.

### Platform support floors

Windows 11 x64; the latest macOS and the one before it; current
mainstream Linux distributions. Nothing older is supported — there is no
Windows XP / 7 / 8 / 10 target and no `-T "v###_xp"` toolset. See
[ADR 0003](../DocsAgents/adr/0003-platform-support-floors.md).

## Installing CMake

- **Windows:** `choco install cmake`, `winget install Kitware.CMake`, or
  the installer from <https://cmake.org/download/> (it also ships with the
  Visual Studio installer).
- **macOS:** `brew install cmake` or `port install cmake`.
- **Linux:** your distribution's package manager, or the official binary
  from the download page.

## Configuring

Run these from the **repository root** (not this `Build/` directory). CI
does exactly this:

```
cmake -B build
cmake --build build
```

`cmake -B build` creates a `build/` directory and generates the project
files there:

- **Windows:** a Visual Studio solution (`build/StepMania.sln`). Open it
  in Visual Studio, or keep using `cmake --build build`.
- **macOS:** a Makefile project by default; pass `-G Xcode` for an Xcode
  project. Also pass `-DCMAKE_OSX_ARCHITECTURES=arm64` (or `x86_64`).
- **Linux:** a Makefile project.

### Build type

Single-config generators (Makefiles, Ninja) need the build type at
configure time:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

`Debug`, `RelWithDebInfo`, and `MinSizeRel` are also available. Release is
what ships and what the maintainer plays; Debug produces a separate
`*_debug` executable with assertions on. Multi-config generators (Visual
Studio, Xcode) pick the configuration at build time instead — e.g.
`cmake --build build --config Release`.

### Re-configuring

If a `CMakeLists.txt` or `.cmake` file changes, just build again — CMake
re-runs itself. If that fails, delete `build/` and configure from
scratch.

## Common options

Defined in `CMake/DefineOptions.cmake`. Pass as `-DWITH_X=ON` / `-DWITH_X=OFF`:

| Option | Default | Purpose |
|---|---|---|
| `WITH_FULL_RELEASE` | OFF | Build as a proper, full release |
| `WITH_WERROR` | OFF | Treat StepMania's own warnings as errors (used by CI) |
| `WITH_LTO` | OFF | Link-time optimization |
| `WITH_SSE2` | ON | SSE2 codegen |
| `WITH_CLUB_FANTASTIC` | OFF | Bundle the Club Fantastic song packs |
| `WITH_CRASH_HANDLER` | ON | Built-in crash reporter (non-Windows option; always on for Windows) |
| `WITH_GLES2` / `WITH_GTK3` | ON | OpenGL ES 2.0 / GTK3 UI (Linux) |
| `WITH_ALSA` / `WITH_PULSEAUDIO` / `WITH_JACK` | ON / ON / OFF | Audio backends (Linux) |

## More

Agent- and contributor-facing build notes, including the CI matrix and
the test story, live in
[`DocsAgents/build.md`](../DocsAgents/build.md).
