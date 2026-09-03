---
type: Subsystem
title: Lua binding layer
description: How C++ objects are exposed to theme Lua — LuaManager, Luna<T> bindings, LuaReference.
tags: [lua, scripting, bindings, theme-api]
resource: src/
---

# Purpose

Themes are Lua. This layer embeds Lua 5.1 (`extern/lua-5.1`) and exposes
engine objects to it. File grouping: `src/CMakeData-data.cmake` "Data
Structures/Lua" + `src/LuaManager.*`.

# Key files

| File | Role |
|---|---|
| `src/LuaManager.*` | `LUA` global; owns the `lua_State`, runs scripts, `Lua::Push`/`Get` helpers, error handling |
| `src/LuaBinding.h/.cpp` | The `Luna<T>` template + macros that make a C++ class scriptable |
| `src/LuaReference.*` | RAII handle to a value in the Lua registry (themes pass closures/tables to C++) |
| `src/LuaExpressionTransform.*` | Lazy per-item Lua expressions (used by option rows, wheels) |
| `src/LuaFunctions.cpp`, `src/LuaHelpers.h` | Free functions exposed globally |
| `Scripts/*.lua` | Engine-side Lua shipped with the game |

# Binding pattern

At the bottom of `Foo.cpp`:

```cpp
class LunaFoo: public Luna<Foo>
{
public:
    static int DoThing( T* p, lua_State* L ) { p->DoThing(); COMMON_RETURN_SELF; }
    LunaFoo()
    {
        ADD_METHOD( DoThing );
    }
};
LUA_REGISTER_CLASS( Foo )
```

- `DEFINE_METHOD( name, expr )` — shorthand for a getter.
- `ADD_GETTER_SETTER` — pairs `get_x` / `set_x`.
- `LUA_REGISTER_DERIVED_CLASS( T, Base )` — inheritance in Lua mirrors C++.
- Actors also need `REGISTER_ACTOR_CLASS( Foo )` (see [actors.md](./actors.md)).

# Adding a Lua API

1. Implement the C++ method on the class.
2. Add a `static int` thunk + `ADD_METHOD` in that class's `Luna*`.
3. Document it in `Docs/Luadoc/LuaDocumentation.xml` — **CI validates this
   XML** (`.github/workflows/ci.yml`). `Docs/Luadoc/Lua.xml` is generated.
4. If it's a new class, `LUA_REGISTER_CLASS` and make sure it's `Push`able.

# Gotchas

- **Do not edit `Docs/Luadoc/` by hand carelessly** — `Lua.xml` is
  generated (see `Docs/` build docs); `LuaDocumentation.xml` is
  hand-maintained and CI-validated. Both must stay well-formed.
- Lua stack discipline in thunks: return the number of results;
  `COMMON_RETURN_SELF` / `COMMON_RETURN_SELF`-style macros handle the
  common cases.
- `LuaReference` must outlive any use; losing it lets Lua GC the value.
- Errors: use `luaL_error` / `LuaHelpers::ReportScriptError`, not
  exceptions across the C boundary.
- Theme-facing message names come from `MESSAGEMAN`
  ([singletons.md](./singletons.md)); actors subscribe in Lua.
- External reference: <https://quietly-turning.github.io/Lua-For-SM5/>.
