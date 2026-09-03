---
type: Playbook
title: Add a Preferences.ini value
description: Add a Preference<T>, give it a default, and surface it in the options UI.
tags: [preferences, prefsmanager, options]
---

# Goal

Add a persistent engine setting that lives in `Preferences.ini` and
(optionally) appears in an options screen.

# When to use

A tunable that should persist across runs and is machine/user scoped
(not per-profile, not per-song). Per-profile settings go on `Profile`;
themable numbers go in metrics ([../subsystems/theming.md](../subsystems/theming.md)).

# Files always touched

| Path | Change |
|---|---|
| `src/PrefsManager.h` | Declare `Preference<T> m_xMyPref;` in the right group |
| `src/PrefsManager.cpp` | Init it in the ctor init list: `m_xMyPref("MyPref", <default>)` |
| `src/ScreenOptionsMasterPrefs.cpp` | Add a `ConfOption` row (only if it goes in the UI) |
| Theme `metrics.ini` / options screen list | Reference the `ConfOption` name (only if in the UI) |

# Steps

1. **Declare** in `src/PrefsManager.h` next to related prefs:
   `Preference<bool> m_bMyPref;` (types used: `bool`, `int`, `float`,
   `RString`, or an enum).
2. **Construct** in `PrefsManager::PrefsManager()` init list:
   `m_bMyPref( "MyPref", false )`. The **string name** is the
   `Preferences.ini` key and the lookup key for
   `IPreference::GetPreferenceByName` — it self-registers on construction.
3. **Read it anywhere:** `PREFSMAN->m_bMyPref` (has `Get()` / `Set()` /
   implicit conversion).
4. **UI (optional):** in `src/ScreenOptionsMasterPrefs.cpp`, add a
   `ConfOption` entry mapping a display name + choices to `"MyPref"`.
   For simple bool/enum use the existing `MovePref` / `MoveMap`
   machinery; for dynamic lists (themes, languages) follow the
   `RequestedTheme` / `Language` examples in that file.
5. **UI wiring:** add the `ConfOption` name to the relevant options
   screen's line list in the theme (`_fallback` metrics), or it will not
   show.
6. **Lua (optional):** if themes need it, expose via the `PREFSMAN`
   binding — separate task, see [expose-lua-api](./expose-lua-api.md).

# Gotchas

- **Name string is the contract.** It is the ini key, the
  `GetPreferenceByName` key, and (for UI) the `ConfOption` `m_sPrefName`.
  A typo = silent default, no error.
- Defaults can be overridden per install by a `Static.ini` /
  defaults node (`IPreference::ReadAllDefaultsFromNode`,
  `PrefsManager.cpp:504`). Don't assume the ctor default is what ships.
- Enum prefs need a `EnumToString`/`StringToX` path; look at an existing
  enum pref (`m_BGFitMode`, `m_HighResolutionTextures`) before inventing.
- Adding a pref does **not** require a cache bump (that's simfiles, not
  prefs).
- `ScreenOptionsMasterPrefs.cpp` is MSVC-warning-heavy already; keep the
  new row in the same style, don't refactor the file.

# Verification

- Windows build green (+ `--SelfTest` headless smoke), then
  commit + push (`AGENTS.md` §4).
- Spot-check to note in the commit: launch, change the option in the UI
  (if added), quit, confirm the value is in `Save/Preferences.ini` and
  re-read on next launch.
- One pref + one simple UI row is low-risk. A pref that changes engine
  behavior broadly (e.g. a new render pref) is higher-risk (§4) — split
  finely, spell out the spot-check.

# History

- 2026-09-02 — created from recon: `Preference`/`IPreference`
  (`src/Preference.h`), pref decls in `PrefsManager.h:139+`, options
  bridge `IPreference::GetPreferenceByName` in
  `ScreenOptionsMasterPrefs.cpp:116`. Not executed end-to-end.
