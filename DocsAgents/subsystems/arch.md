---
type: Subsystem
title: Platform abstraction (arch)
description: Per-OS driver implementations selected at build time — window, input, sound, movie, lights, dialogs, threads.
tags: [arch, platform, windows, macos, linux, drivers]
resource: src/arch/
---

# Purpose

`src/arch/` holds the OS-specific implementations behind Rage's abstract
interfaces. `src/archutils/` holds OS helper code (crash handlers,
special dirs, registry, etc.) split into `Win32/`, `Darwin/`, `Unix/`,
`Common/`. The right driver is chosen by CMake
(`src/CMakeData-arch.cmake`, `src/CMakeData-os.cmake`) and at runtime.

# Pattern

Each folder has an abstract base + `_<Platform>` implementations, and
usually a `_Null` no-op fallback:

| Folder | Interface | Implementations |
|---|---|---|
| `ArchHooks/` | app lifecycle, time, messagebox | `_Win32`, `_MacOSX`, `_Unix` |
| `LowLevelWindow/` | GL context + window | `_Win32`, `_MacOSX`, `_X11` |
| `LoadingWindow/` | splash during boot | `_Win32`, `_MacOSX`, `_Gtk`, `_GtkModule`, `_Null` |
| `InputHandler/` | raw input devices | `_DirectInput` (Win), `_Linux_Event`/`_Joystick`/`_tty`/`_PIUIO`, `_MacOSX_HID`, `_X11`, `_MonkeyKeyboard` |
| `Sound/` | audio output | `_DSound_Software`/`_WaveOut` (Win), `_ALSA9_Software`/`_PulseAudio`/`_JACK`/`_OSS` (Linux), `_AU` (mac), `_Null` |
| `MovieTexture/` | video decode → texture | `_FFMpeg`/`_Generic`, `_DShow` (Win), `_Null` |
| `Lights/` | arcade cabinet lights | many `LightsDriver_*` (PacDrive, Minimaid, PIUIO, ITGIO, Parallel, Export…) |
| `MemoryCard/` | USB profile storage | `_Linux`, `_Windows`, `_MacOSX`, `_Null` (all `Threaded_Folder`-based) |
| `Dialog/` | error/prompt dialogs | `_Win32`, `_MacOSX` |
| `Threads/` | thread primitives | `_Pthreads`, `_Win32` |

# Gotchas

- macOS files are often `.mm` (Objective-C++). Windows-only helpers live
  in `src/archutils/Win32/` and pull in `windows.h`.
- Adding a device/driver = new `_<Platform>` file **plus** wiring in the
  matching `CMakeData-*.cmake` / `CMakeData-arch.cmake` and the driver's
  factory (`*Driver.cpp` `Create*` / `RegisterDriver`).
- `WITH_*` CMake options gate many drivers (e.g. `WITH_PACDRIVE`,
  `WITH_ALSA`, `WITH_JACK`) — see [../build.md](../build.md).
- Crash handling: `src/archutils/*/Crash*` + `WITH_CRASH_HANDLER`.
- This is where most "works on X, broken on Y" bugs live.
