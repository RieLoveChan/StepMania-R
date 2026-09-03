---
type: Playbook
title: Expose a C++ method to theme Lua
description: Add a scriptable method (or class) to the Luna<T> binding so themes can call it.
tags: [lua, bindings, theme-api]
---

# Goal

Make `someObject:MyMethod(args)` callable from theme/NoteSkin Lua.

# When to use

A theme needs to read or drive engine state that is not yet scriptable.
Not for engine-internal logic (keep that in C++).

# Files always touched

| Path | Change |
|---|---|
| `src/<Class>.cpp` | Add the `static int` thunk + `ADD_METHOD` in the `Luna<Class>` block (bottom of file) |
| `src/<Class>.h` | Only if the underlying C++ method is new |
| `Docs/Luadoc/LuaDocumentation.xml` | **CI-validated** — see Gotchas |

# Steps

1. **Find the binding.** Bottom of `src/<Class>.cpp`:
   `class Luna<Class>: public Luna<Class> { ... }` then
   `LUA_REGISTER_CLASS( <Class> )` (or `LUA_REGISTER_DERIVED_CLASS`).
   Reference example: `LunaBitmapText` in `src/BitmapText.cpp`.
2. **Write the thunk:**
   ```cpp
   static int MyMethod( T* p, lua_State* L )
   {
       // args: IArg(1), FArg(2), SArg(3), etc. (1-based)
       p->MyMethod( IArg(1) );
       COMMON_RETURN_SELF;           // or: lua_pushnumber(L, x); return 1;
   }
   ```
3. **Register it** in the `Luna<Class>()` constructor: `ADD_METHOD( MyMethod );`
   - Getter/setter pair: `ADD_GETTER_SETTER( x )` → `get_x` / `set_x`.
   - Pure getter shorthand: `DEFINE_METHOD( GetX, GetX() )`.
4. **New class?** Add `LUA_REGISTER_CLASS( NewClass )` and make sure it is
   `Push`able (`LuaManager` / `PushSelf`).
5. **Document** the method in `Docs/Luadoc/LuaDocumentation.xml` so CI's
   `xmllint --noout` passes and Lua-For-SM5 stays accurate.

# Gotchas

- **CI runs `xmllint --noout Docs/Luadoc/Lua.xml` and
  `LuaDocumentation.xml`** (`.github/workflows/ci.yml`, job
  `validate-xml-docs`). Malformed XML fails the build. `Lua.xml` is
  *generated*; `LuaDocumentation.xml` is hand-edited.
- `LuaDocumentation.xml` is hand-edited and is the theme-facing API
  contract — update it whenever you add/change a method, keep it
  well-formed (CI fails otherwise).
- **Stack discipline:** return the number of results you pushed.
  `COMMON_RETURN_SELF` returns `self` (1). Getting this wrong = Lua
  crashes or silent nil.
- Do not throw C++ exceptions across the Lua boundary — use `luaL_error`
  / `LuaHelpers::ReportScriptError`.
- Arg helpers assert on wrong types; guard optional args with
  `lua_isnoneornil(L, n)`.
- Actors additionally need `REGISTER_ACTOR_CLASS` (see
  [../subsystems/actors.md](../subsystems/actors.md)).
- If the method should fire theme messages, go through `MESSAGEMAN`.

# Verification

- Windows build green + `validate-xml-docs` job green.
- Manual: call the method from a scratch theme screen / `lua.dump`, or
  `ScreenSandbox`, and confirm behavior.
- Usually a **small change** (one file + doc) → normal review. Becomes
  **large** if it exposes a whole new class or many methods at once.

# History

- 2026-09-02 — created from `src/LuaBinding.h` macros and the
  `LunaBitmapText` example; CI XML validation confirmed in `ci.yml`.
