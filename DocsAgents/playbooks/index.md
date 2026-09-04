# Playbooks

Repeatable procedures for recurring tasks. A playbook exists so the next
agent does **not** rediscover the steps, the files that always change, or
the traps. Read the relevant one before starting; update it after.

If a task you did needed more than one correction, or hit a non-obvious
dead end, and it is something that will recur — add or fix a playbook
here (see [`_template.md`](./_template.md)). One-off specifics go in the
subsystem doc's `# Gotchas` instead.

# Available

### Feature work
* [add-ssc-tag](./add-ssc-tag.md) - Add a new field to the `.ssc` simfile format (loader, writer, cache).
* [expose-lua-api](./expose-lua-api.md) - Make a C++ class or method callable from theme Lua.
* [add-screen](./add-screen.md) - Add a new `Screen*` state to the engine.
* [add-preference](./add-preference.md) - Add a `Preferences.ini` value and surface it in options.

### Modernization
* [add-characterization-test](./add-characterization-test.md) - Pin an engine function's current behaviour with a Catch2 test under `tests/` before refactoring it.
* [clang-tidy-subsystem-pass](./clang-tidy-subsystem-pass.md) - Run one modernization pass over a single subsystem: fix, verify Windows build, commit + push.
* [migrate-rstring](./migrate-rstring.md) - Convert one subsystem from `RString` to `std::string` without a repo-wide diff.
* [split-god-object](./split-god-object.md) - Carve a cohesive slice out of `GameState` / `ScreenEdit` / `GameManager` with zero call-site churn in phase 1.

# Wanted (not written yet)

* fix-memory-leak - Confirm and fix an entry from the cppcheck list (see [../modernization-backlog.md](../modernization-backlog.md) item 3).
* harden-c-string - Replace an unsafe `strcpy`/`strcat` cluster with bounded copies (backlog item 8).
