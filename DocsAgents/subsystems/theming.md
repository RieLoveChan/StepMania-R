---
type: Subsystem
title: Theming system
description: ThemeManager, metrics.ini, the Themes/ tree, actor templates, localization and paths.
tags: [theming, metrics, themes, localization]
resource: Themes/
---

# Purpose

Almost all UI layout, screen flow, strings, colors and behavior live in
**themes**, not C++. `ThemeManager` (`THEME`) resolves metrics and asset
paths with fallback.

# Key pieces

| Thing | Where | Notes |
|---|---|---|
| `ThemeManager` | `src/ThemeManager.*` | `THEME` global. `GetMetric*`, `GetPathToG/S/B/O` (graphic/sound/bg/other), `GetString` (localization) |
| `ThemeMetric<T>` | `src/ThemeMetric.h` | Typed, self-registering metric accessor cached per class |
| `CommonMetrics` | `src/CommonMetrics.*` | Shared metric names |
| `LocalizedString` | `src/LocalizedString.*` | Translated string handle |
| Theme data | `Themes/<name>/` | `metrics.ini`, `Languages/*.ini`, `Graphics/`, `Sounds/`, `BGAnimations/`, `Scripts/*.lua`, per-screen folders |
| `_fallback` theme | `Themes/_fallback/` | Base every theme inherits from; defines default screens/metrics |
| Base assets | `_assets/Themes/` | Source art |

# Resolution / fallback

`THEME` looks up `[ClassName] MetricName` in the current theme's
`metrics.ini`, then walks `FallbackTheme` (ultimately `_fallback`).
Paths resolve current-theme → fallback → `_missing`. Languages layer
`Languages/en.ini` under the active language.

# Gotchas

- **Changing a menu = edit the theme, not `src/`.** Grep `Themes/` (and
  `Themes/_fallback/`) for the metric or Lua file first.
- `metrics.ini` uses `::` command syntax and Lua expressions; many values
  are `lua,...` closures evaluated per use.
- `Themes/` in this repo is largely git-ignored beyond the shipped ones
  (see `.gitignore` "StepMania Specific" block: `Themes/pump*`, `test`,
  `moonlight`, …). The canonical editable theme content is `_fallback`
  and whatever default ships.
- Adding a C++ tunable → expose it as a `ThemeMetric<T>` with a sane
  `_fallback` default, don't hard-code.
- User-visible text → `LocalizedString` / `THEME->GetString`, English key
  in `_fallback/Languages/en.ini`.
- Reference: `Docs/Themerdocs/` (do not edit).
