---
type: Playbook
title: Migrate one subsystem from RString to std::string
description: Retire RString in a bounded scope without a repo-wide diff, keeping boundaries with un-migrated code intact.
tags: [modernization, rstring, strings]
---

# Goal

Remove `RString` from one subsystem's `.cpp`/`.h`, replacing it with
`std::string`, while every call site that crosses into not-yet-migrated
code still compiles and behaves identically.

# When to use

Per ADR 0001 (Settled #5): `RString` retirement is **opportunistic and
per-subsystem**, never a global campaign. Do this when you are already
working in a subsystem for another reason, or as a dedicated small PR for
a leaf subsystem with few external string interfaces.

# Background

- `RString` is `typedef StdString::CStdString RString;` (`src/global.h:107`).
  `CStdString` is `CStdStr<char>`, a subclass of `std::basic_string<char>`
  — so it *is* a `std::string` plus extra methods.
- Scale: ~723 files, ~8,429 uses. This is why it cannot be one diff.
- Extra API that `std::string` lacks (the porting surface):
  `.Format(...)`, `.MakeUpper()` / `.MakeLower()`, `.Left(n)` /
  `.Right(n)` / `.Mid(...)`, `.Trim*()`, `.SpanExcluding(...)`, plus some
  implicit `const char*` conveniences.

# Files touched

Only the chosen subsystem's files, plus its `DocsAgents/subsystems/*.md`
if a boundary gotcha turned up, plus `log.md`.

# Steps

1. **Scope tight.** One subsystem (`DocsAgents/subsystems/` boundary).
   Prefer one with few functions that take/return strings across the
   boundary. Count them first: `rg -n "RString" <files>`.
2. **Map the extra API to `RageUtil` free functions** (they already
   exist for non-`RString` use):
   | `RString` method | Replacement |
   |---|---|
   | `s.Format("%d", x)` | `s = ssprintf("%d", x)` |
   | `s.MakeUpper()` / `MakeLower()` | `MakeUpper(s)` / `MakeLower(s)` (RageUtil) |
   | `s.Left(n)` | `s.substr(0, n)` |
   | `s.Right(n)` | `s.substr(s.size()-n)` (guard `n <= size`) |
   | `s.Mid(i, n)` | `s.substr(i, n)` |
   | `s.Trim()` | `Trim(s)` (RageUtil) |
   | `s.SpanExcluding(set)` | `RageUtil` equivalent / manual |
3. **Replace `RString` → `std::string`** in the subsystem's own decls,
   locals, members, and internal signatures.
4. **At the boundary:** functions called from un-migrated code — keep the
   signature accepting/returning something both sides accept. Since
   `CStdString` derives from `std::string`, a function taking
   `const std::string&` still accepts an `RString` argument from a caller.
   Returning `std::string` where callers expect `RString` is usually fine
   (implicit construct). Verify each boundary function compiles from the
   caller side.
5. **Do not touch** `global.h`, `StdString.h`, or the `RString` typedef.
   It stays until the last subsystem is done (a final ADR retires it).
6. **Build Windows**, review every hunk, keep formatting local.

# Gotchas

- **A `const RString&` (or `RString&`) parameter is a hard boundary,
  not a soft one.** The playbook's "boundary is usually fine" claim
  (step 4) is about the *safe* direction: passing an `RString` where
  `const std::string&`/`std::string` (by value) is expected, or
  returning `std::string` where a caller stores it into an `RString`
  variable — both work because `RString` derives from `std::string`
  and has a converting constructor from it (`StdString.h:361`). The
  **reverse** direction does not: a plain `std::string` argument cannot
  bind to a parameter typed `const RString&`/`RString&` (reference to
  the *derived* type), since that would require an implicit
  base-to-derived conversion, which doesn't exist. If a not-yet-
  migrated function you call takes `RString` by reference (not by
  value, not `const std::string&`), a `std::string` argument needs an
  explicit `RString(...)` wrap at the call site — found in practice
  migrating `Command.cpp` (`Difficulty.h`'s `StringToDifficulty(const
  RString&)`, fixed with 2 explicit wraps in `UnlockManager.cpp`).
- **Containers don't inherit the boundary safety.** `std::vector<Derived>`
  has no relationship to `std::vector<Base>` — there's no container
  covariance in C++. If a subsystem's own vector/map of strings is
  passed to a not-yet-migrated function expecting
  `std::vector<RString>&` (e.g. `RageUtil`'s `split`/`join`), you
  cannot migrate that container's element type without also migrating
  the function it's passed to. Keep such internal containers as
  `RString` and only migrate the *scalar* values that cross the
  boundary (a single string return/copy, not the container itself) —
  this is what kept `Command.cpp`'s `m_vsArgs` as `std::vector<RString>`
  while still migrating the public API's individual string returns.
- **`printf`-family + `std::string`:** `ssprintf("%s", s.c_str())` — a
  bare `std::string` into a varargs `%s` is UB. `RString` sometimes
  papered over this; do not carry the bug.
- `RString` had case-insensitive-ish helpers and `operator==` with
  `const char*`; `std::string` comparisons are exact. Watch equality
  checks that relied on `CStdString` conveniences.
- Serialization / `IniFile` / `XmlFile` / MSD parsing APIs are heavy
  `RString` users — a subsystem that talks to them has a big boundary;
  pick a different subsystem first.
- clang-tidy `modernize-*` string checks may misfire while `RString` is
  still in the translation unit — run tidy *after* the migration, not to
  drive it.
- Lua bindings (`SArg`, `lua_pushstring`) take `const char*` / build
  `RString` — check the `Luna*` block if the subsystem has one.

# Verification

- Windows build green, no new warnings, `--SelfTest` smoke green, then
  commit + push (`AGENTS.md` §4).
- Higher-risk (cross-file type migration): commit **per subsystem**, one
  subsystem per commit, so a revert is a single clean undo. Spell out in
  the message what to spot-check — string-heavy code is easy to break
  with subtle formatting/trim differences.

# History

- 2026-09-02 — created from recon: `global.h:107`, `StdString.h` API
  surface, usage counts. No subsystem migrated yet — first executor
  fills in a "known-good order" list of subsystems here.
- 2026-09-12 — first subsystem migrated: `Grade.cpp`/`Grade.h` (10
  `RString` mentions, the smallest leaf subsystem in the codebase).
  Confirmed the boundary-safety claim in step 4 holds in practice with
  zero caller-side changes across 8 caller files: `CStdStr(const
  std::string&)` (`StdString.h:361`) makes `std::string`→`RString`
  implicit, and `CStdString`'s inheritance from `std::basic_string<char>`
  makes `RString`→`const std::string&` implicit. The one gotcha hit:
  `.MakeUpper()`/`.MakeLower()` are `CStdString`-only methods with no
  direct `std::string` equivalent — use the `RageUtil` free function
  guarded exactly like `CStdStr::MakeUpper()` itself does:
  `if(!s.empty()) MakeUpper(&s[0], s.size());`. Full gate green
  (`sm_tests` unchanged, `[Grade]` tag unchanged, `ctest`, Release,
  `--SelfTest`). **Known-good order so far:** start with the smallest
  leaf files first (`RString` mention count, not file size) — good
  next candidates are similarly small single-purpose util/data files
  with few cross-file string-returning functions.
- 2026-09-12 — second subsystem migrated: `Command.cpp`/`Command.h`
  (~20 `RString` mentions). Migrated only the *scalar* boundary values
  (`GetName()`, `GetOriginalCommandString()` on both `Command` and
  `Commands`, the `Arg::s` member) to `std::string`; deliberately kept
  internal storage (`m_vsArgs`, the `Load()`/`ParseCommands()`
  parameters, the `split()`/`join()` calls) as `RString` per the new
  container gotcha above. Found the hard-boundary gotcha above in
  practice (`Difficulty.h`'s `StringToDifficulty(const RString&)`
  needed 2 explicit `RString(...)` wraps in `UnlockManager.cpp`). Full
  gate green (`sm_tests` unchanged, `[Command]` tag 26/6 unchanged,
  `ctest`, Release, `--SelfTest`).
- 2026-09-12 — third subsystem migrated: `ScoreDisplayCalories.cpp`/
  `.h` (1 `RString` member). Trivial and fully self-contained — the
  hard-boundary wrap (`MessageManager::Unsubscribe(const RString&)`)
  landed in the same file, no other caller file needed touching.
  **New lesson: `RString` mention count alone doesn't make a good
  pilot signal — check what each mention actually does.** Scouted and
  rejected `MeterDisplay.cpp`/`ComboGraph.cpp` (both immediately
  forward a by-value `RString` param into `AutoActor`/`ThemeManager`/
  `ThemeMetric` APIs — large un-migrated subsystems, no real reduction
  in footprint) and `PlayerAI.cpp` (feeds straight into
  `IniFile`/`XNode`, already named in this playbook's own "heavy
  RString users" warning). A file with 1-2 `RString` mentions is only
  a good pilot if those mentions stay local (a member field, a return
  value) rather than passing straight through to a big subsystem's
  reference-taking API. Full gate green (`sm_tests` unchanged, `ctest`,
  Release, `--SelfTest`; no dedicated characterization test exists for
  this widget).
- 2026-09-13 — fourth subsystem migrated: `StyleUtil.h`/`.cpp`'s
  `StyleID` class (`sGame`/`sStyle`, private members). **Prefer private
  members when scouting** — they have zero external call-site exposure
  by construction, so the whole boundary-safety analysis is confined to
  the owning `.cpp`. Found a new hard-boundary shape: `XNode`'s
  `GetAttrValue(const RString&, T &out)` is templated on `T`, so it
  *looks* generic, but it forwards to `XNodeValue::GetValue(T &out)`,
  which is **not** templated — only 5 concrete virtual overloads exist
  (`RString&`/`int&`/`float&`/`bool&`/`unsigned&`), so `T = std::string`
  fails to resolve. A templated wrapper isn't proof the underlying call
  is generic; check what it actually calls, especially anything backed
  by non-template virtual dispatch (`XNodeValue` underlies both
  `IniFile` and `XmlFile` — watch for it elsewhere too). Fixed with a
  local `RString` temporary for the read side; the write side
  (`AppendAttr`/`SetValue(const RString&)`, by-value in the template)
  needed no change, since `std::string`→`RString` construction is
  always the safe direction. Full gate green (`sm_tests` unchanged,
  `ctest`, Release, `--SelfTest`).
- 2026-09-13 — fifth subsystem migrated: `CourseUtil.h`/`.cpp`'s
  `CourseID` class. **A private member can still have external exposure
  through a public getter that returns a reference to it** —
  `const RString &GetPath() const { return sPath; }` can't keep
  returning a reference once the member is `std::string` (a
  `const RString&` can't reference a base-typed object). Prefer
  dropping the reference (return the accessor's type *by value*
  instead) over changing the getter's return type to
  `const std::string&`, especially when the getter has few callers —
  a by-value `RString` return still binds directly to any
  `const RString&`-taking caller with zero changes there, whereas a
  `const std::string&` return type would need every caller re-checked
  for the same hard-boundary issue this whole gotchas section is about.
  Also: `.Left(n)`/`.Right(n)`/`.Mid(i,n)` (the mapping table above)
  really are exact behavioral matches, not approximations — confirmed
  via `StdString.h:500`, `CStdStr::Left` is itself implemented as a
  clamped `substr(0, n)`. `CourseID` is §5-adjacent (`.crs` course-file
  identity) — re-verified `[crs]` (39/5) unchanged before/after, on top
  of the usual gate.
- 2026-09-13 — sixth subsystem migrated: `SongUtil.h`/`.cpp`'s `SongID`
  class. Checking a family of similarly-named ID classes together
  (`SongID`/`StepsID`/`TrailID`/`CourseID`/`StyleID`) pays off: found
  `TrailID` has **zero `RString` members at all** (nothing to
  migrate — checking before assuming a same-named-family class is a
  candidate saves wasted analysis), and `StepsID`'s `sDescription` is
  private with no exposing getter, a clean pilot waiting for a future
  round. `SongID::sDir` was identically shaped to `CourseID::sPath`
  (same two `.Left(n)` sites, same `XNode::GetAttrValue` hard-boundary
  fix). `SongID` is §5-adjacent (core song identity) — re-verified
  `[corpus]` (313/3) unchanged before/after.
- 2026-09-13 — seventh subsystem migrated: `StepsUtil.h`/`.cpp`'s
  `StepsID::sDescription`, closing out this round's `*ID` family scout.
  A **second, different flavor of hard-reference-boundary parameter**
  turned up: `SongUtil::GetOneSteps(...)`'s `sDescription` parameter is
  `const RString &sDescription` — a real reference parameter, unlike
  the by-value `RString` parameters (`StringToGame`, `GetCourseFromName`,
  `GetSongFromDir`, etc.) every earlier `*ID` pilot happened to hit.
  Same fix as always (an explicit `RString(...)` wrap at the one call
  site), but a reminder that **by-value vs. by-reference isn't
  predictable from the pattern of prior pilots — check every call's
  actual signature, every time**, even after the same fix has worked
  several times in a row on parameters that turned out to be by-value.
- 2026-09-13 — eighth subsystem migrated: `CryptHelpers.h`/`.cpp`'s
  `RSAKeyWrapper::Load(...)`. **A new shape: migrating a public
  function's own parameter types, not an internal member behind an
  unchanged interface.** This is usually the bigger, more externally-
  visible kind of change, but stays small when the function has few
  callers (here: 4, all in one file, `CryptManager.cpp`) — changed
  `Load`'s params from `(const RString&, RString&)` to
  `(const std::string&, std::string&)` directly, and **every existing
  `RString` argument/out-param variable at the call sites bound with
  zero changes**, because `RString`'s inheritance from `std::string`
  makes a derived-typed variable satisfy a base-typed reference
  parameter automatically — the same safe direction used everywhere
  else in this playbook, just applied to a function's own declared
  signature instead of storage behind it. Verified the blast radius
  really was that contained by checking the rebuild only recompiled 2
  files, not a wider cascade.
- 2026-09-13 — ninth subsystem migrated: `ScoreKeeper.h`/`.cpp`'s
  `MakeScoreKeeper(...)` factory method — a **by-value** parameter
  version of pilot #8's shape, simpler still, since a by-value
  parameter accepts either an `RString` or `std::string` caller
  argument unambiguously. Only 1 caller. Also scouted `Trail.h`'s
  public `Modifiers` member and **deliberately stopped short of
  migrating it**: it's touched from 3 external files, and one call,
  `PlayerOptions::FromString(const RString&)`, is a real reference
  parameter (needs a wrap); worse, `Message::SetParam(const RString&,
  const T&)` is a template forwarding to `LuaHelpers::Push(L, val)` —
  possibly the same "templated wrapper, non-template backing" trap as
  `XNode::GetAttrValue`, not yet confirmed. **Don't half-migrate a
  candidate on the assumption a template is generic — verify
  `LuaHelpers::Push`'s actual overload set first if this one is picked
  up again.**
- 2026-09-13 — tenth subsystem migrated: `TrailEntry::Modifiers`
  (`Trail.h`/`.cpp`), unblocking pilot #9's flagged candidate. Checked
  `LuaHelpers::Push`'s overload set (`LuaManager.cpp:93`) and found a
  **genuine `std::string` template specialization already exists**
  alongside the `RString` one — unlike `XNode::GetAttrValue`, this
  template really is generic; `Message::SetParam` calling it is safe
  with no wrap needed. **Not every templated wrapper hides a
  non-template trap — check the actual specializations before assuming
  either way, in both directions.** Needed 3 `RString(...)` wraps for
  genuine reference-parameter hard boundaries
  (`Attack::FromGlobalCourseModifier(const RString&)`,
  `PlayerOptions::FromString(const RString&)` called from 2 different
  files) — a public struct member touched from multiple external files
  can still be a good pilot, it just costs one wrap per hard-boundary
  call site, not per file. §5-adjacent — re-verified `[corpus]` (313/3)
  and `[crs]` (39/5) both unchanged.
- 2026-09-13 -- scouting methodology bug found and fixed: attempted
  `CommandLineActions::CommandLineArgs::argv`
  (`std::vector<RString> argv;`), scouted its external callers with
  `grep -rn ... src/*.cpp` and found only one, in `src/*.cpp` itself --
  looked clean. **The shell glob `src/*.cpp` does not recurse into
  subdirectories**, so it silently missed
  `src/archutils/Win32/GraphicsWindow.cpp`, which passes `args.argv`
  by reference into `split(..., std::vector<RString>&, ...)` -- the
  exact container hard-boundary already known from the `Command.cpp`
  pilot. The actual build (which does compile the whole tree, `arch`/
  `archutils` included) caught this immediately with a compile error
  before anything was committed, so no broken commit resulted -- but
  the scouting step itself gave a false "looks safe" signal. **Always
  scout with a genuinely recursive search across all of `src/`
  (`grep -rn ... src/` with no glob, or an explicit recursive flag),
  never a single-directory glob** -- `src/arch/` and
  `src/archutils/Win32/` are real, separate directories this codebase
  keeps platform code in, and a scout that only checks `src/*.cpp`
  will never see callers living there. Reverted the attempted change
  (net zero diff, nothing to commit) once the real boundary was found.
- 2026-09-13 -- eleventh subsystem migrated (found while scouting
  GameState.h for item-9 god-object clusters, not a dedicated RString
  scouting pass): GameState::m_RandomAttacks
  (std::vector<RString> -> std::vector<std::string>). A container
  member, but a direct type change was enough here -- unlike
  Command.cpp's m_vsArgs or CommandLineActions::argv, no function
  anywhere takes this whole vector by reference to an un-migrated
  vector<RString>& parameter, so it needed none of the container-
  boundary workarounds those earlier cases required. Only 2 callers
  (Player.cpp, SongManager.cpp), both using plain container methods.
  A reminder that container members ARE migratable when nothing
  external passes them through by reference -- check every caller's
  actual usage pattern (method calls on elements/the container itself
  vs. passing the whole container onward) before assuming a container
  member is automatically off-limits.
- 2026-09-13 -- twelfth subsystem migrated (item 9's low-risk
  god-object clusters ran out, pivoted back to item 10):
  `PlayerStageStats::FormatPercentScore(float)`'s own return type
  (`RString` -> `std::string`). Same "migrate a public function's own
  signature" shape as pilot #8/#9, but for a *return* type instead of
  a parameter. New confirmation: `CStdStr` has a genuine
  `operator=(const std::string&)` overload (`StdString.h:391`), not
  just the base-class `std::basic_string<char>::operator=` -- so
  assigning a `std::string`-returning function's result into an
  existing `RString&`/`RString` variable is always safe, the same
  "safe direction" as passing a `std::string` where an `RString`
  parameter is expected. Only 1 real caller outside the `LuaFunction`
  registration (`PaneDisplay.cpp`), and that registration's
  `LuaHelpers::Push` forwarding was already confirmed generic in pilot
  #10, so this pilot needed zero `RString(...)` wraps anywhere.
- 2026-09-13 -- thirteenth subsystem migrated: `CodeSet.h`/`.cpp`'s
  `InputQueueCodeSet::Load(...)` parameter and `::Input(...)` return
  type. **A macro can hide a hard-boundary call just as well as a
  function body can** -- `Load`'s `CODE_NAMES`/`CODE(s)` macros expand
  to `THEME->GetMetric(sType, ...)`, a real `const RString&` reference
  parameter into the still-`RString` `ThemeManager`; grep for the
  parameter name alone won't show this unless the macro body is read
  too. Fixed with an `RString(sType)` wrap inside each macro
  definition itself (2 wraps, both confined to the one `.cpp`).
  Reconfirmed the `split(...)` container hard-boundary from the
  `Command.cpp`/`CommandLineActions` pilots: `m_asCodeNames`
  (`std::vector<RString>`) stays `RString` because
  `split(const RString&, const RString&, std::vector<RString>&, bool)`
  (`RageUtil.h:406`) has no `std::vector<std::string>&` overload --
  every `RageUtil` free function that fills a caller-provided container
  needs checking the same way before assuming a container member is
  migratable.
- 2026-09-13 -- fourteenth subsystem migrated: `RandomSample.h`/`.cpp`'s
  `Load`/`LoadSoundDir`/`LoadSound`, all three by-value `RString`
  parameters. Straightforward multi-hard-boundary case: 5 separate
  `RString(...)` wraps needed in one function body
  (`GetExtension`/`GetDirListing` x4) since `RageUtil`'s directory/
  path helpers are still `RString`-only, but every wrap stayed local to
  `RandomSample.cpp` itself. Reconfirmed `RageSound::Load(RString
  sFile)` (a *different*, still-un-migrated subsystem's by-value
  parameter) needs no wrap when called with a migrated
  `std::string` argument -- by-value parameters accept either type
  from either direction, so a pilot's own by-value migration never
  needs to wait for a downstream by-value call to migrate too.
- 2026-09-13 -- fifteenth subsystem migrated:
  `RageUtil_WorkerThread.h`/`.cpp`'s `RageWorkerThread` constructor
  parameter and private `m_sName` member. Reconfirmed the by-value
  safe direction one more time, but through string *concatenation*
  this time: `"\"" + sName + "\" worker event"` (now `std::string`
  arithmetic) still produces a plain `std::string`, which binds
  straight into `RageEvent(RString name)`'s by-value parameter with no
  wrap -- string concatenation results are just as safe to pass by
  value into an un-migrated by-value parameter as a bare variable is.
- 2026-09-13 -- sixteenth subsystem migrated: `RageSurfaceUtils.h`/
  `.cpp`'s `SaveSurface`/`LoadSurface` by-value `file` parameter.
  Routine shape by now: `RageFile::Open(const RString&, int)` is a
  real reference-parameter hard boundary, fixed with one
  `RString(file)` wrap per function; the single external caller file
  passes a `const RString` local, safe by the by-value rule. No new
  wrinkle -- this pilot mainly confirms the "migrate a function's own
  by-value parameter, wrap at any reference-parameter call sites in
  the body" pattern is now fully mechanical to execute once a
  candidate's external footprint is scouted.
