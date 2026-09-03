# Compiling and installing StepMania-R

Read [`README.md`](./README.md) first and configure the build. This
document covers compiling the generated project and, optionally,
installing it to a system location.

## Compiling

From the repository root:

```
cmake --build build --config Release
```

Or open the generated project in your IDE and build there:

- **Windows:** open `build/StepMania.sln` in Visual Studio and build.
- **macOS:** build the Makefile project, or open the Xcode project if you
  configured with `-G Xcode`.
- **Linux:** `cmake --build build` (or `make -C build`).

The executable is written to `Program/` in the repository root
(`Program/StepMania-R.exe` on Windows; a `*_debug` variant for Debug
builds). On macOS the bundle is `StepMania-R.app`; on Linux the binary is
`stepmania` plus an optional `GtkModule.so`.

## Running from the build tree

**No install step is needed for development or for just playing.** The
binary runs in place from the repository root, next to `Themes/`,
`NoteSkins/`, `Songs/`, `Data/`, and the rest of the content folders.

## Installing (optional)

"Installing" copies the binary and content folders to a standard system
location. This is only for packaging or a system-wide deployment.

```
cmake --install build --config Release
```

The destination is CMake's default prefix (`C:\Program Files\StepMania` on
Windows, `/usr/local` on Unix). Override it at configure time:

```
cmake -B build -DCMAKE_INSTALL_PREFIX=/your/path
```

- **Windows:** the binary lands in `<prefix>\Program\`, content folders
  alongside.
- **macOS:** copy `StepMania-R.app` to `/Applications`; it runs as-is.
- **Linux:** installs under `<prefix>/stepmania/`, with a
  `stepmania.desktop` entry and `Installer/setup.sh`.

Packaging (`.exe` installer, `.dmg`, archives) is driven by CPack — see
`CMake/CPackSetup.cmake`.
