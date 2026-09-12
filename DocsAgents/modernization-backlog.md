---
type: Reference
title: Modernization backlog
description: Ranked list of code needing attention, from a 2026-09-02 source-tree sweep. The live to-do for the fork.
tags: [modernization, backlog, triage]
---

# How to use

Ranked by "does this block safe continuous work" first, then rot, then
risk, then hotspots. Tiers 1–2 are step 2 (make change safe + cheap
cleanup). Tier 3 needs ADR decisions. Tier 4 is where ongoing passes go.

Update this file as items are closed (strike through + link the PR) and
as new sweeps find things. Numbers/anchors confirmed 2026-09-02.

---

## Tier 1 — Blocks safe continuous work

**Tier 1 is now empty.** Items 1–3 all closed: the safety net (smoke +
Catch2 harness + characterization coverage of the pure-ish cores,
2026-09-05), the MSVC-warning ratchet (2026-09-11, all 5 categories
promoted — see item 2 below), and the 2009 cppcheck leak list
(2026-09-05).

### 27. `sm_tests` crashed on the first engine assert on Unix — FIXED 2026-09-10
Diagnosed in a Docker `ubuntu:24.04` container with gdb. Root cause was
**not** the crash handler: `EngineTestEnv` brought up engine singletons
without loading a theme, and many engine ctors (`Song`, `SongManager`,
`ThemeMetricStepsTypesToShow::Read`, ...) read `ThemeMetric<T>` values
via `GetValue()`, which does `ASSERT_M( m_Value.IsSet() )`. With no
theme that assert fires → `sm_crash()` → on Unix `_exit(1)` (Windows
silently tolerates the unset `LuaReference` and returns a default,
which is why it was Windows-green and Unix-red).
**Fix:** `EngineTestEnv` now loads **`SMRTest`**, a scripts-free
minimal theme (`tests/data/test-theme/`, `FallbackTheme=`, no
`Scripts/`). `SwitchThemeAndLanguage` runs almost nothing; metrics it
doesn't define resolve "missing → `Dialog::ignore` → nil". Also needed:
build `NOTESKIN` before `GAMEMAN->GetDefaultGame()`, give `GAMESTATE` a
current game (`SetCurGame`) before the theme load, and
`Dialog::SetWindowed(false)` so the per-missing-metric
`AbortRetryIgnore` doesn't pop a modal MessageBox on Windows.
Fixed a real Linux bug found along the way: `find_package(Iconv)` never
set `HAVE_ICONV`, so `RageUtil_CharConversions.cpp` fell to its
"no converters" `#else` on Linux and **silently blanked non-UTF-8 song
titles/artists** (Korean KSF, Japanese BMS, CP1252 DWI). Now
`HAVE_ICONV` is defined when iconv is found and not Apple (Apple keeps
its CoreFoundation branch); added an `ICONV_CONST` fallback define.
Full suite green on Windows **and** Linux (`5966 / 226`); the CI
`continue-on-error` on the Unix test jobs was removed.

### 1. Safety net — DONE 2026-09-05
- **Headless smoke: DONE** (`f7249f3a95`) — `--SelfTest` flag runs full
  engine init and exits 0; wired into Windows CI (`continue-on-error`
  until it's green a few times, then make fatal).
- **Harness scaffold: DONE** (merged `5_1-new` 2026-09-04). ADR
  [0006](./adr/0006-test-harness.md) (Catch2 v3 + `sm_engine` OBJECT
  library); `sm_tests` + first `RageUtil` coverage; CI green on
  Windows/macOS/Linux (see item 17).
- **Unit coverage: DONE 2026-09-05.** Characterization targets, in order:
  `RageUtil` → `RageMath` (2026-09-04) → `TimingData` (2026-09-05 —
  row/measure math + beat<->time conversion via the GAMESTATE/PREFSMAN-
  free `NoOffset` entry points) → `NoteData` (2026-09-05 — tap/hold
  storage, track queries, row traversal; the counting/statistics API
  needs a live GAMESTATE and is out of scope, see item 17) →
  `NoteDataUtil` (2026-09-05 — the pure transform helpers) →
  `NotesLoader*` (2026-09-05 — `tests/test_NotesLoader.cpp`: the
  parse *primitives* — `MsdFile` tokenizer,
  `GetMainAndSubTitlesFromFullTitle`, `SMLoader::RowToBeat`/
  `ParseBPMs`/`ParseStops`/`Process{BPMsAndStops,Delays,
  TimeSignatures,Tickcounts}` on valid input. Full `LoadFromDir`/
  `LoadFromSimfile` needs live `FILEMAN`+`LUA` → stays `--SelfTest`
  smoke; the helpers' `LOG->UserLog` error branches need a live `LOG`.
  A committed simfile corpus is a future add if/when the harness grows
  an engine-bootstrap fixture). **311 assertions / 72 cases.**

### 2. Warnings on but unmeasured / unenforced — DONE 2026-09-11, all 5 MSVC categories promoted
`WITH_WERROR` exists (default OFF, ON in Windows CI) and counts are
captured in [`baseline.md`](./baseline.md) (ADR 0001 §7 mechanism).
**Done (2026-09-04):** `C4189` (unused local, was 26) and `C4702`
(unreachable code, was 10) promoted to `-Werror` — all 15 unique hit
sites fixed by hand first (dead locals removed; two fully-dead
computation blocks deleted; degenerate `FOREACH_X(v) return ...;`
loops — which MSVC flags because the body always returns on the first
iteration, making the loop's back-edge provably unreachable — rewritten
as a direct `GetNextX()` + `if`; one `[[noreturn]]`-followed-by-dead-code
cleanup). Verified clean under `/WX` in both Debug and Release (Debug
surfaced 3 more sites Release's optimizer had folded away — always
check both configs, not just Release).
**Done (2026-09-05):** `C4100` (unreferenced parameter). Real measured
count was **314 unique sites** (not the stale ~1362 raw-line figure,
which double-counts headers across every including TU) spread across
~150 files with no concentration (max 11 in one file) — a wide
mechanical sweep across five commits (`713e58a0f6`, `9844ab3c4b`,
`26b7de158b`, `3636b80690`, `c1d77d662a`), highest-concentration files
first down to the 62 single-site files last. Convention: comment out
the unused parameter name (`Type /* name */`, this codebase's existing
style), except one site genuinely used only under
`#if defined(HAVE_POSIX_FADVISE)` (unset on Windows) —
`[[maybe_unused]]` there instead, since commenting the name would
break the other platform's compile. Several sites looked like false
positives but weren't: verify each site's actual body rather than
assume the pattern — used by a *different* override of the same
virtual in the same file, or usage only inside a `/* doesn't work */`
block comment, or a near-duplicate signature where only one of two
overloads/declarations actually triggered the warning.
`/wd4100` **removed from `src/CMakeLists.txt` permanently** —
promoted to `-Werror` alongside `C4189`/`C4702`. Verified with a full
Release rebuild (not just the touched files) showing zero `C4100`
across all of `src/`, plus `sm_tests` clean under the existing
`WITH_WERROR=ON` config.
**In progress (started 2026-09-11):** `C4244`/`C4267` (numeric
conversion / narrowing). The old "~4.4k hits" figure was stale/raw
(same double-counting problem C4100's ~1362 had). Real counts need
`/wd4244 /wd4267` actually **removed** from `src/CMakeLists.txt` before
measuring — a `--clean-first` rebuild *with the suppression still in
place* will show 0 no matter what's fixed, which cost an hour of false
signal mid-sweep; always check the flag is gone before trusting a 0.
With it removed, a clean `sm_engine`/`sm_tests` (Debug, `WITH_TESTS=ON`,
`WITH_WERROR=ON`) rebuild, counting unique `file(line,col)` sites:
- **Release `StepMania` target: 0 sites**, entirely accounted for by
  4 lines in two headers everything includes (`RageUtil.h`/
  `RageTimer.h`) — fixed first (see below), Release never had more.
- **Debug/tests, before this session's fixes: ~297 sites.** After
  fixing `NoteField.cpp` (real count 35), `TimingSegments.cpp` (5, not
  25 — `push_back(int)` into a `vector<float>` at 5 call sites;
  MSVC's template-instantiation "note: with _Ty=..." context lines
  inflate the apparent per-file count but not the true `(line,col)`
  dedup), `RageSurfaceUtils.cpp` (6, not 22, same template-note
  inflation), `ScreenOptionsMasterPrefs.cpp` (2, not 18, one generic
  `if constexpr` helper instantiated for many `T`/`U`): 187 remained,
  then fixing `NoteDataWithScoring.cpp` (11), `NoteDisplay.cpp` (9),
  `ScreenGameplay.cpp` (9, two of them from the shared
  `FLOAT_TABLE_INTERFACE` macro in `OptionsBinding.h` -- fixed once at
  the macro, benefits every other user of it too), `NetworkManager.cpp`
  (6), `Profile.cpp` (10) and `NotesLoaderBMS.cpp` (10, §5-adjacent
  `NotesLoader*` -- characterization only, `test_NotesLoaderBMS.cpp`'s
  26 assertions / 2 cases re-checked identical): **a second methodology
  trap** — checking a fix by deleting just the touched `.obj` files and
  rebuilding (not `--clean-first`) only recompiles those files plus
  their direct dependents, so it *undercounts the total* (it did catch
  that the touched files themselves were clean, correctly). The
  authoritative number needs `--clean-first` **and** the flag removed
  together. With both: 140 sites appeared to remain -- **but this was
  itself wrong, a third methodology trap:** the dedup regex used to
  count unique sites (`warning C424[47]`) only matches `C4244`
  (`"C424"`+`"4"` or `"7"`); `C4267` is `"C426"`+`"7"`, a different
  literal string the same regex can never match. Every "140/69" figure
  in this doc and in `log.md`/memory silently counted **C4244 only**
  and dropped every `C4267` warning -- 838 raw `C4267` lines, 388
  unique sites, simply never counted. Corrected regex
  (`warning C42(44|67)`) on the same clean rebuild log: **528 unique
  sites across 152 files** was the real total at that point (before
  the batch below). Top concentrations at that point:
  `ScreenDebugOverlay.cpp`/`RageFileBasic.cpp`/`BitmapText.cpp`/
  `ActorMultiVertex.cpp` (each previously reported as "5", actually
  5/11/7/16 once `C4267` was counted too), then `RageUtil.cpp` (25),
  `SongManager.cpp` (20), `XmlFileUtil.cpp` (14), `ThemeManager.cpp`
  (11), `ScreenEdit.cpp` (11), `RageFileManager.cpp` (11),
  `OptionsList.cpp`/`EditMenu.cpp`/`CubicSpline.cpp` (10 each),
  `TimingData.cpp`/`MusicWheel.cpp`/`CourseLoaderCRS.cpp` (9 each).
  After fixing all real sites in the four files above (the C4267 sites
  included): **489 unique sites remain across 148 files** (re-verified
  clean in the same rebuild). This is the current authoritative count
  -- treat any earlier "140/69" or "297"/"187"/"158" figure in this doc
  as superseded; they were never wrong about the C4244 sites they
  tracked, just silently blind to C4267 the whole time.
**Done, verified (`sm_tests` 5966/226, `ctest`, Release + `--SelfTest`
green, `/wd4244`/`/wd4267` still in place while sites remain elsewhere):**
- `RageUtil.h`/`RageTimer.h`: the 4 header sites (`RageTimer::
  GetTimeSinceStartFast`, `MersenneTwister`'s seed, `RandomFloat`,
  `FindIndex`'s iterator-diff return).
- `NoteField.cpp`: two macros fixed at their single definition site
  covered ~30 of the 35 call-site warnings — `draw_all_segments`'s
  `side_sign= ... ? -1 : 1` (int ternary into a `float`, now `-1.f`/
  `1.f`) and `IS_ON_SCREEN`'s two `float`-member-into-`int`-parameter
  args to `IsOnScreen()` (now `static_cast<int>`). Plus a handful of
  individual call sites passing the same float members to `int`
  parameters (`DrawBoard`, `FindFirstDisplayedBeat`/
  `FindLastDisplayedBeat`), an int-subtraction-into-float, an
  int-round-tripped-through-`(int)`-then-back-into-a-float member, and
  a `lua_tonumber()` (`lua_Number`/double) truncated to an index `int`.
- `TimingSegments.cpp`: `GetCombo()`/`GetMissCombo()`/`GetNum()`/
  `GetDen()`/`GetUnit()` (ints/enum) pushed into `std::vector<float>
  GetValues()` results — cast each.
- `RageSurfaceUtils.cpp`: `std::trunc(float)` (still a `float`) assigned
  to an `int` pixel index ×2; a clamped `std::lrint` (`long`) assigned
  to `std::uint8_t` ×2.
- `ScreenOptionsMasterPrefs.cpp`: `FindClosestEntry<T,U>`'s `if
  constexpr` helper lambda — one `static_cast<T>` around its call fixes
  every `T`/`U` instantiation at once.
- `NoteDataWithScoring.cpp`: `RadarValues` (float-backed) read as `int`
  counts (`out[RadarCategory_Notes]` etc.) and int scoring counters
  (`state.taps_hit` etc.) written back into it — cast both directions.
  §5-adjacent (`NoteData*`); characterization-only (no logic change),
  verified against the existing `sm_tests` coverage.
- `NoteDisplay.cpp`: degrees-as-double (`PI_180`/`PI_180R`) multiplied
  into a `float` rotation; the same `IsOnScreen()` float-members-into-
  int-params pattern as `NoteField.cpp`; a pointer-difference
  (`v - buf`) into `int` ×2.
- `ScreenGameplay.cpp`: int assigned from `SafeFArg()` (returns `int`)
  into `float margins[][2]` ×3; an iterator-diff into `int`; an enum
  subtraction into `int` ×2; `lua_pushnumber(size_t)` (should be
  `lua_Number`) ×1 direct + 2 more via the shared macro below.
- `OptionsBinding.h`'s `FLOAT_TABLE_INTERFACE` macro: `size()` (`size_t`)
  into `lua_createtable`'s `int` count param, `n+1` (`size_t`) into
  `lua_rawseti`'s `int` index param, and a bare `size_t` into
  `lua_pushnumber`'s `lua_Number` param — fixed once at the macro
  definition, so every screen/options-page that uses this interface
  benefits, not just `ScreenGameplay.cpp`.
- `NetworkManager.cpp`: `lua_tointeger()` (`lua_Integer`) into `int`
  timeout/interval fields ×4; `lua_pushnumber(uint64_t)` (should be
  `lua_Number`) ×2.
- `Profile.cpp`: `GetAge()`'s year-difference into `float`; a
  `std::count_if` (`ptrdiff_t`) return into `float`;
  `CalculateCaloriesFromHeartRate`'s unit-conversion chain (×4); plus
  (only surfaced on a genuine clean rebuild) `size()`/iterator-diff/
  `size()-1` values (`size_t`) into `int` ×6 across high-score checks,
  `RString::Right(int)`, a reverse file-index loop (safe by
  construction: an empty vector's `SIZE_MAX` still truncates to `-1`),
  and two `lua_createtable`/`lua_rawseti` pairs.
- `NotesLoaderBMS.cpp` (§5-protected `NotesLoader*`; every cast here is
  characterization-only, no logic change): a `double` measure-size
  assigned to a `float` field ×2; `long long` beat-fraction num/den
  into `SetTimeSignatureAtRow`'s `int` params ×2; a `double` sum into
  `BeatToNoteRow`'s `float` param; an `unsigned int` BPM into a `float`
  field; plus (only surfaced on a genuine clean rebuild) four more
  `size()`/`find()` (`size_t`) values assigned into `int`/`unsigned`
  locals. Re-verified via `sm_tests.exe "[bms]" -s`: 26 assertions / 2
  cases, all pinned values unchanged (e.g. `pnm-nine`'s tap count still
  118).
- `ActorMultiVertex.cpp` (16 sites, once `C4267` was counted): Lua
  bindings pushing `std::size_t` (`GetNumVertices`/`GetState`/
  `GetNumQuadStates`/`GetQuadState`/vertex-loop index) into
  `lua_pushnumber`'s `lua_Number`/`lua_rawgeti`'s `int`; a `size_t`
  modulo/subtraction into `int` locals (`spi`, `max`, `size`, `Last`);
  `VertexIndex` (`size_t`) into three `SetVertex*(int index, ...)`
  setters; a `size_t` return cast into `SetState(int)`/`AddVertices
  (int)`.
- `BitmapText.cpp` (7, once `C4267` was counted): `std::fmin(1, ...)`
  promoting an `int`+`float` mix to `double` (fixed by using the `1.0f`
  literal instead of a cast, so the whole macro stays `float`); a
  `size_t` (`m_vpFontPageTextures.size()`/`m_aVertices` count) into
  `int` locals ×2; `rnd()%N` (`unsigned int`) into `RageVector3`'s
  `float` ctor args ×2; `lua_tointeger()` into `Attribute::length`
  (`int`, not `size_t` -- checked the header).
- `RageFileBasic.cpp` (11, once `C4267` was counted): a pointer-diff
  into `int` ×2 (constructor buffer offset, `FillReadBuf`'s available-
  space calc); `std::size_t iBytes` (a `Write`/`Read` parameter) added
  into or divided against `int` members/locals across `EmptyWriteBuf`/
  `Write`/the 3-arg `Write` overload -- 6 sites, all `static_cast<int>`
  on the `iBytes` operand (these are internal buffer sizes, always well
  under `INT_MAX` in practice).
- `ScreenDebugOverlay.cpp` (5, no additional `C4267` beyond the
  original `C4244` count): three `const_iterator - begin()`
  pointer-diffs into `int` page/subscriber indices; a `double`
  (`RageTimer::GetTimeSinceStart()`) into `SecondsToMMSSMsMsMs`'s
  `float` param.
**Second methodology trap, corrected:** checking a fix by deleting only
the touched `.obj` files and rebuilding recompiles just those files
(plus header-dependents), not the whole tree — it can't surface sites
in code paths the narrow recompile never touches, so treating its "0
warnings for these files" as a full-tree count silently undercounts
everywhere else too. The `NetworkManager`/`Profile`/`NotesLoaderBMS`
batch above was first "confirmed" this way, then a real
`--clean-first` rebuild (flag genuinely removed) turned up several
more real sites in those same three files (now all fixed and
re-confirmed at zero in that same clean rebuild).
- `RageUtil.cpp` (25): `int len`/`int size` parameters and locals
  (`SmEscape`/`DwiEscape`/`DelimitorLength`/`do_split`'s begin+size
  bookkeeping) fed from `RString::size()`/pointer-diffs; two identical
  `pcre_exec(..., sStr.size(), ...)` calls (`int` length param);
  `Trim*`/`Dirname`'s size-backed loop counters; `Json::Value::resize`
  (`ArrayIndex`, not `size_t`); a `TONUMBER_NICE` macro and
  `multiapproach`'s `mult` local losing `lua_tonumber`'s `double` into
  `float`; five `lua_rawgeti`/`lua_rawseti`/process-index sites in
  `multiapproach`'s per-element loop (`size_t i` into `int`).
- `SongManager.cpp` (20): `SetTotalWork`/`wrap`/`RString::Left`/`Right`
  size-into-int at load-time call sites; eight one-line `GetNum*()`
  getters returning `size()`/`count_if()` (`ptrdiff_t`) as `int`; three
  reverse-loop `size()-1` bounds (safe by construction, same pattern as
  `Profile.cpp`); a `count_if` accumulated into `int iCount`.
- `XmlFileUtil.cpp` (14): a local `SetString(int,int,...)` helper
  called from 6 sites with `RString::size_type` (`size_t`) start/end
  positions -- cast at each call site rather than widening the helper's
  params, since its internal `iEnd-1 >= iStart` bounds check relies on
  signed underflow behavior that unsigned params would change; two
  reverse-loop `size()-1` bounds.
- `ThemeManager.cpp` (11): `RString::Left`/`Right` fed `size_t`
  positions from `.find()`/`.size()` arithmetic; a reverse-loop
  `size()-1` bound and a `size()`-returning getter; two
  `std::distance()` (`ptrdiff_t`) results into `int iDist`; a
  `lua_createtable`/`lua_rawseti` pair pushing theme-fallback names.
- `ScreenEdit.cpp` (11): `RandomInt(size_t)`; `SCREEN_HEIGHT/480*0.5`
  promoting to `double` via integer division then multiply, fixed with
  an outer `static_cast<float>` (×2 identical sites); an `int` beats-
  per-measure value into a `float`; a `choices.size()-1` default-choice
  index; a `find()-begin()` pointer-diff into `unsigned pos` plus a
  `size()` into a keysound-index `int`; a `MenuRowDef` size-derived
  count param; two `size_t`-indexed track-remap array writes; a
  `size()` into `numKeysounds`.
- `RageFileManager.cpp` (11): a `Seek(int)` fed an `mz_uint64` file
  offset (the call already runtime-checks for truncation via a
  round-trip compare, so the cast doesn't change behavior); reverse-
  loop `size()-1` bounds ×3; `RString::Left`/`Right` fed
  `size_t`-derived mount-point-length arithmetic; three `size()`/
  `length()` values into `int` locals tracking driver/file counts.
- `OptionsList.cpp` (10): `unsigned`/`int` locals and a `GetScreen(int)`
  call fed `size_t` choice counts/indices; two `wrap(int&, size_t)`
  sites (the count arg force-cast); a linear-search loop returning its
  `size_t` index as `int`.
- `EditMenu.cpp` (10): two reverse-loop `size()-1` bounds; a
  `switch`/`case` block of six one-line `GetRowSize` accessors
  returning `size()` as `int`; a `size()-1` default-selection index.
- `CubicSpline.cpp` (10): a shared `LCSN_EVAL_SOMETHING` macro's
  `lua_createtable`/`lua_rawseti` pushing per-point float vectors --
  fixed once at the macro (covers 4 call sites); three more
  `lua_createtable`/`lua_rawgeti`/`lua_rawseti` sites in the
  coefficient get/set helpers; two `lua_pushnumber(size_t)` accessors
  (`get_size`/`get_dimension`).
- `TimingData.cpp` (9, §5-protected `TimingData`, characterization-
  only): a 4-way `size()` sum into an `unsigned int` segment count; an
  `EraseSegment(int, ...)` fed a `size_t` loop index at 2 call sites; a
  binary-search `size()-1` upper bound; a `lua_createtable`/
  `lua_rawseti` block serializing timing segments to Lua (2 tables, 3
  index sites).
- `MusicWheel.cpp` (9): a `MusicWheelItemData` ctor's `int` section-
  count param fed `size_t` at 2 sites; two `unsigned` snapshot-size
  locals; a `std::min<unsigned int>` call's second arg; two
  `wrap(int&, size_t)` sites; a reverse-loop bound; a
  `RandomInt(size_t)` call.
- `CourseLoaderCRS.cpp` (9, §5-protected `.crs`, characterization-
  only): four `RString::Left`/`Right` pairs (8 sites) fed
  `strlen()`/`size()`-derived `size_t` lengths when parsing
  `BEST`/`WORST`/`GRADEBEST`/`GRADEWORST` song-choice prefixes; an
  `int` (`StringToInt`) assigned into a `float` field.
**A third file-level discovery en route:** the broken `C424[47]` regex
had also silently hidden residual `C4267` sites in three files this
doc had already marked "done, verified" earlier in the sweep --
`NoteField.cpp` (5: two `int` column-index members fed a `size_t` loop
var, a binary-search `size()-1` bound, a `lua_createtable`/
`lua_rawseti` pair pushing column-renderer actors), `ScreenGameplay.cpp`
(2: an `unsigned int` queue-size local, a reverse-loop `size()-1`
bound), `ScreenOptionsMasterPrefs.cpp` (1: a `MoveMap(..., unsigned
cnt)` call fed `size_t`). All three re-confirmed at zero in the same
clean rebuild that produced the numbers below; a full sweep of every
previously-"done" file turned up no further residuals.
- `RageDisplay_OGL.cpp` (8): a shader-compile helper's `GLint`/
  `GLsizei` length/count args fed `RString::size()`; two `int`-into-
  `float` args to `LoadMenuPerspective`; two `std::max<unsigned int>`
  calls fed `size_t` vertex/triangle totals; a `size_t` old-buffer-size
  local.
- `Song.cpp` (7, §5-protected `Song*`, characterization-only): four
  reverse-loop `size()-1` bounds across `RemoveAutoGenNotes`/
  `DeleteSteps`/`FreeAllLoadedFromProfile`; a `lua_createtable`/
  `lua_rawseti` pair serializing background changes to Lua. **One of
  the four reverse-loop sites was missed on the first pass** (an `Edit`
  call reporting "2 matches" for the same bare pattern only fixed one
  of the two ambiguous occurrences by adding disambiguating context --
  the other, identically-shaped occurrence a few lines later was never
  revisited) and only caught by the next clean-rebuild verification;
  fixed and reconfirmed at zero.
- `ScreenSelectMaster.cpp` (7): a `lua_pushnumber(size_t)` site; a
  `lua_rawgeti` fed a `size_t` loop counter; a `SET_POS_PART` macro's
  `lua_tonumber`-into-`float` (fixed once, covers 3 uses); two
  `wrap(int&, size_t)` sites; a `size_t`-into-`int` local.
- `RageLog.cpp` (7): a `double`-into-`float` timestamp arg; four
  `size_t`/`strlen()` values accumulated into `unsigned` log-buffer
  counters; a `std::min<unsigned int>` call's second arg; a
  `std::min(size_t, size_t)` result into an `int` log-size local.
- `RageFileDriverDeflate.cpp` (7): a `size_t` byte-count assigned into
  zlib's `uInt avail_out`/`avail_in` fields at 2 sites; two pointer-
  diffs into `int` (inflate progress tracking); a `size_t` returned as
  `int` from `WriteInternal`.
- `RageDisplay.cpp` (7): a `std::round()` result into an `int` refresh
  rate; a `std::ceil()` result into an `int` width; a `size_t` vertex
  count into `StatsAddVerts(int)`; four mesh-info `size_t` counts/
  accumulators into `int` fields.
- `XmlToLua.cpp` (6): `RString::Left`/`Right` fed `size_t` arithmetic
  at 5 call sites parsing field names and file paths (`add_extension_
  to_relative_path...`, `store_cmd`, `store_field`, `load_frames_
  from_file` ×2); a trailing-`.lua` filename `Left()` call.
- `WheelBase.cpp` (6): a `size_t` item count into `int`; five
  `wrap(int&, size_t)` sites across `ChangeMusic`/`RebuildWheelItems`/
  `FirstVisibleIndex`.
- `StatsManager.cpp` (6): a `size_t` song count into `unsigned`; a
  `std::trunc()` result and two `size_t` accumulators feeding profile
  gameplay-seconds/songs-played counters; a `size_t`-minus-`int` index;
  a `lua_pushnumber(size_t)` accessor.
- `ScreenOptions.cpp` (6): a `MoveRowAbsolute` call's `size_t`-minus-1
  arg; the same `const int iNumChoices = ...m_vsChoices.size()`
  pattern at 2 call sites (4 warnings, both `int`/`const int`
  variants); a `wrap(int&, size_t)` site.
- `Course.cpp` (6): a `size_t` mod-change count into `int`; two
  `RandomInt(size_t)` calls choosing steps; two identical
  `std::floor((iMinDist + iMaxDist) / 2)` sites promoting an all-`int`
  expression to `double` then narrowing back, in two near-duplicate
  meter-balancing code paths; a `lua_pushnumber(size_t)` accessor.
- `ActorMultiTexture.cpp` (6): two `int`-into-`float` texture-size
  members; two `size_t`-into-`int` texture-unit-count returns; two
  `enum_add2(TextureUnit_1, size_t)` calls needing an `int` second arg.
- `RageFileDriverMemory.cpp` (5): a memory-file `int m_iFilePos` fed
  `size_t` byte counts across `Read`/`WriteInternal`, plus a
  `GetFileSize()` returning the backing buffer's `size_t`.
- `NoteDataUtil.cpp` (5, §5-adjacent `NoteData*`, characterization-
  only): a pointer-diff-via-`intptr_t` into `int` (unmatched-hold
  warning arg); a `size_t` length into a `LoadFromSMNoteDataString
  WithPlayer` `int len` param; a `set<int>::size()` added into an
  `int` track-pressed count; a `size_t` subtracted from a taps-left
  counter; a reverse-loop `size()-1` bound.
- `LuaManager.cpp` (5): the `FromStack<int>`/`FromStack<unsigned int>`
  specializations' `lua_tointeger()`-into-narrower-type assignments;
  a `lua_objlen()` into an `int` thread-pool index; two more
  `lua_tointeger()`-into-`int` sites (`AdjustCount`, `lua_pushvalues`'s
  upvalue arg count).
- `CryptManager.cpp` (5): five `hash_descriptor[].process`/
  `rsa_verify_hash_ex` calls fed a `size_t` buffer length where
  libtomcrypt's C API expects `unsigned long`.
**Not done:** `/wd4244`/`/wd4267` stay in `src/CMakeLists.txt` until all
233 remaining sites (~120 files) are triaged (same "fix everything,
then remove the `/wd` flag in one commit" pattern as C4100) — continue
file-by-file, highest concentration first; measure only via
`--clean-first` with the flag actually removed, dedup with a regex
that matches BOTH `C4244` and `C4267` (`warning C42(44|67)`, not
`C424[47]`), never run the measurement rebuild while a file is
mid-edit, watch for `Edit` "N matches" errors resolved by fixing only
one of several truly-identical occurrences (the `Song.cpp` miss above),
and periodically re-grep every already-"done" file against a fresh
clean-rebuild log (careful: a plain filename grep like `Course.cpp(`
also matches `ScreenOptionsEditCourse.cpp(` -- a substring false
positive, not a residual; anchor or eyeball matches before treating them
as real). Still not measured for Clang/GCC (`baseline.md` TBD,
non-Windows).
- `StepMania.cpp` (4): a `ceil()` result into a window-width `int`; a
  `MEMORYSTATUS::dwTotalPhys` division into an `int`; an `srand(
  time(nullptr))` seed narrowing.
- `SongUtil.cpp` (4): two `SecondsToMMSS(int)` calls in a length-sort
  label; an `RString::Left` fed a `size_t`-minus arg; a reverse-loop
  bound.
- `ScreenUnlockStatus.cpp` (4): two `size_t` unlock counts into
  `unsigned`; two reverse-loop bounds (the file's own comment explains
  why they're `int`, not `unsigned`, loop vars).
- `ScreenServiceAction.cpp` (4): two near-duplicate edit-clearing
  helpers each with a `size_t` file count and a `count_if` result into
  `int`.
- `ScreenJukebox.cpp` (4): three `RandomInt(size_t)` calls; a
  reverse-loop bound.
- `ScoreKeeperNormal.cpp` (4): a `size_t` song-count into `int`; two
  `lua_tointeger()`-into-`int` sites in a toasty-trigger callback.
- `RageTimer.cpp` (4): two `Difference()` (returns `double`) results
  narrowed to the class's `float`-returning API; a `std::floor(float)`
  result into `int64_t` seconds.
- `RageSoundReader_Preload.cpp` (4): four `m_Buffer->size() /
  framesize` sites (a repeated frame-count computation) into `int`.
- `RageSoundReader_MP3.cpp` (4): three MAD-stream pointer-diffs into
  `int`; an `id3_tag_query` buffer-length arg into `id3_length_t`.
- `RageMath.cpp` (4): a ternary using `double` literals where the
  function returns `float` (fixed with `f`-suffixed literals instead of
  a cast); three `double`-typed intermediate results in a triangle-wave
  helper cast at each `return`.
- `RageDisplay_D3D.cpp` (4): a palette-index map lookup's `size_t`
  value into `int`; two `std::max<unsigned int>` calls fed `size_t`
  vertex/triangle totals (same pattern as the earlier OGL fix).
**Not done:** `/wd4244`/`/wd4267` stay in `src/CMakeLists.txt` until all
189 remaining sites (~110 files) are triaged (same "fix everything,
then remove the `/wd` flag in one commit" pattern as C4100) — continue
file-by-file, highest concentration first; measure only via
`--clean-first` with the flag actually removed, dedup with a regex
that matches BOTH `C4244` and `C4267` (`warning C42(44|67)`, not
`C424[47]`), never run the measurement rebuild while a file is
mid-edit, watch for `Edit` "N matches" errors resolved by fixing only
one of several truly-identical occurrences, and periodically re-grep
every already-"done" file against a fresh clean-rebuild log (watch for
substring false positives on short/common filenames).
- `OptionRow.cpp` (4): a `size_t` course-entry count into `int`; a
  `std::min<unsigned int>` call's second arg; a `lua_pushnumber
  (size_t)` accessor.
- `JsonUtil.h` (4): four near-identical `root.resize(v.size())` sites
  across `SerializeVectorObjects`/`SerializeVectorPointers` (×2)/
  `SerializeArrayValues` template helpers, each needing
  `Json::Value::ArrayIndex` instead of `size_t`.
- `GraphDisplay.cpp` (4): a `size_t` vertex count into `DrawQuads`'s
  `int`; a `size_t`/`int` division into a fan count; two theme-metric
  `int`s into `float` size members.
- `AdjustSync.cpp` (4): a `size_t` size snapshotted before/after a
  filter call into an `int` counter (both sides of the diff); a
  `size_t` into `unsigned int`; a `FormatNumberAndSuffix(int)` call fed
  a `size_t` loop index.
- `ActorScroller.cpp` (4) + `DynamicActorScroller.cpp` (2, bonus find
  while in the file): a `size_t` sub-actor count into `int`; two
  `std::ceil(float)` results into `int`; three `wrap(int&, size_t)`
  sites across both scroller files.
- `Actor.cpp` (4): two `RString::Left` calls fed `size_t`-minus
  arithmetic parsing command/message names; a `uint64_t` timer value
  into a `generic_global_timer_update(float, ...)` call; a
  `lua_pushnumber(size_t)` accessor.
**Not done:** `/wd4244`/`/wd4267` stay in `src/CMakeLists.txt` until all
163 remaining sites (~105 files) are triaged (same "fix everything,
then remove the `/wd` flag in one commit" pattern as C4100) — continue
file-by-file, highest concentration first; measure only via
`--clean-first` with the flag actually removed, dedup with a regex
that matches BOTH `C4244` and `C4267` (`warning C42(44|67)`, not
`C424[47]`), never run the measurement rebuild while a file is
mid-edit, watch for `Edit` "N matches" errors resolved by fixing only
one of several truly-identical occurrences, and periodically re-grep
every already-"done" file against a fresh clean-rebuild log (watch for
substring false positives on short/common filenames). Next
concentrations: a wide tail of 19 files at 3 sites each
(`WheelNotifyIcon.cpp`/`UnlockManager.cpp`/`Steps.cpp`/
`ScreenSelectCharacter.cpp`/`ScreenSelect.cpp`/`Screen.cpp`/
`RageThreads.cpp`/`RageSoundReader_ChannelSplit.cpp`/
`RageFileDriverReadAhead.cpp`/`RageFileDriverDirect.cpp`/
`PlayerStageStats.cpp`/`OptionRowHandler.cpp`/`NotesLoaderSSC.cpp`/
`LightsManager.cpp`/`InputQueue.cpp`/`GameManager.cpp`/
`ArrowEffects.cpp`/`ActorFrame.cpp`, plus `ScreenOptionsEditCourse.cpp`
which is a genuinely separate file from `ScreenOptions.cpp`/
`Course.cpp` and not yet touched).
**All 19 fixed in one batch (57 sites):** every site was a `size_t`/
`int64_t`/`double`/`lua_Integer`-family value narrowed into a smaller
`int`/`unsigned`/`float`/`lua_Number` type at a `wrap()` call, a
reverse-loop bound, an iterator-diff, a `lua_push*`/`lua_raw*` Lua
binding, or (×2, both characterization-only) a §5-protected file:
`Steps.cpp` (a Lua `GetColumnCues` binding) and `NotesLoaderSSC.cpp`
(a `RadarValues` index, a combo-tag size, a `Left()` tag-prefix
compare). No logic changes anywhere; `sm_tests` unchanged.
**Every remaining 2-site file fixed in one batch (26 files, 52
sites):** `StepsUtil.cpp`, `StageStats.cpp`, `Sprite.cpp`,
`ScrollBar.cpp`, `ScreenSelectMusic.cpp`,
`ScreenOptionsManageProfiles.cpp`, `ScreenMapControllers.cpp`,
`ScreenInstallOverlay.cpp`, `SampleHistory.cpp`, `RandomSample.cpp`,
`RageUtil_CharConversions.cpp`, `RageSoundReader_SpeedChange.cpp`,
`RageSoundReader_Resample_Good.cpp`, `RageSoundReader_Chain.cpp`,
`RageModelGeometry.cpp`, `RageFileDriverTimeout.cpp`,
`ProfileManager.cpp`, `PlayerOptions.cpp`, `Player.cpp`,
`NotesLoaderSMA.cpp` (§5-protected, characterization-only),
`NotesLoaderJson.cpp` (§5-protected, characterization-only),
`NoteData.cpp` (§5-protected `NoteData*`, characterization-only),
`MsdFile.h`, `GameSoundManager.cpp`, `CsvFile.cpp`,
`CharacterManager.cpp` — same recurring patterns throughout: `size_t`/
`double`-family values narrowed at `wrap()`/`RandomInt()` calls,
iterator-diffs, reverse-loop bounds, `RString::Left`/`Right` length
args, and Lua `lua_push*`/`lua_raw*` bindings. No logic changes;
`sm_tests` unchanged.
**Not done:** `/wd4244`/`/wd4267` stay in `src/CMakeLists.txt` until all
54 remaining sites (all now single-site files) are triaged (same "fix
everything, then remove the `/wd` flag in one commit" pattern as
C4100) — measure only via `--clean-first` with the flag actually
removed, dedup with a regex that matches BOTH `C4244` and `C4267`
(`warning C42(44|67)`, not `C424[47]`), never run the measurement
rebuild while a file is mid-edit, watch for `Edit` "N matches" errors
resolved by fixing only one of several truly-identical occurrences,
and periodically re-grep every already-"done" file against a fresh
clean-rebuild log. Remaining 54 files (all headers or `.cpp`, 1 site
each): `WorkoutGraph.cpp`, `WheelBase.h`, `Tween.cpp`, `TrailUtil.cpp`,
`ScreenTextEntry.cpp`, `ScreenOptionsManageEditSteps.cpp`,
`ScreenOptionsManageCourses.cpp`, `ScreenHighScores.cpp`,
`ScreenEvaluation.cpp`, `ScreenEnding.cpp`, `ScreenDimensions.cpp`,
`ScreenDemonstration.cpp`, `RageUtil_FileDB.cpp`,
`RageTextureManager.cpp`, `RageSurface_Load_XPM.cpp`,
`RageSoundUtil.cpp`, `RageSoundReader_Vorbisfile.cpp`,
`RageSoundReader_Merge.cpp`, `RageSoundReader_Chain.h`,
`RageSoundMixBuffer.h`, `RageFileDriverDirectHelpers.cpp`,
`RageFileDriver.cpp`, `Profile.h`, `PrefsManager.cpp`, `OptionRow.h`,
`NotesWriterSSC.cpp`, `NotesWriterSM.cpp`, `NotesLoaderSM.cpp`,
`NotesLoaderKSF.cpp`, `NotesLoaderDWI.cpp`, `NoteData.h`,
`MsdFile.cpp`, `ModelTypes.cpp`, `MenuTimer.cpp`,
`MemoryCardManager.cpp`, `InputMapper.cpp`, `IniFile.cpp`,
`HighScore.cpp`, `GameState.cpp`, `GameConstantsAndTypes.cpp`,
`FontCharAliases.cpp`, `Font.cpp`, `EnumHelper.cpp`,
`CryptHelpers.cpp`, `CourseUtil.cpp`, `CourseContentsList.cpp`,
`Course.h`, `Character.cpp`, `BackgroundUtil.cpp`, `Background.cpp`,
`Attack.cpp`, `AnnouncerManager.cpp`, `ActorMultiVertex.h`,
`ActorFrame.h`. Several of these (`NotesLoaderSM.cpp`,
`NotesLoaderKSF.cpp`, `NotesLoaderDWI.cpp`, `NotesWriterSSC.cpp`,
`NotesWriterSM.cpp`, `NoteData.h`, `Profile.h`, `Course.h`) are
§5-protected — characterization-only casts, re-verify against the
matching `test_NotesLoader*.cpp`/existing coverage before considering
each done. All 54 fixed and reconfirmed clean.
**A fourth methodology trap, caught right at the finish line:** every
"clean, edit-free `--clean-first` rebuild" this sweep used a dedup
regex anchored on `^[A-Za-z]:` that silently stopped matching any path
containing a space or parenthesis -- which includes the default MSVC
toolchain install path (`C:\Program Files (x86)\...`), so a genuine
site instantiated from our code but attributed by the compiler to a
*standard-library header* line (e.g. `<algorithm>`'s `std::transform`
body) was invisible to every count this whole sweep. Worse: every
"complete" clean rebuild had also been silently stopping exactly at
the alphabetical end of the root `src/*.cpp` file list and never
reaching the `src/arch/` and `src/archutils/Win32/` subdirectories at
all -- not a regex problem, a genuinely incomplete build each time,
never previously suspected because the tail of the log always looked
plausible (ending at `XmlFileUtil.cpp`) and the file count was never
cross-checked against the source tree. Both gaps were caught only by
this session's final full-tree rebuild, which reached and warned on
files never seen in any earlier count in this sweep. Fixed:
`Song.cpp`'s two `std::transform(..., ::tolower)` calls (the
`<algorithm>`-attributed site -- `int`-returning C library function
assigned through a `char` iterator, fixed with a casting lambda,
incidentally also fixing the classic tolower-with-negative-char UB
gotcha); 14 files / 25 sites across `src/arch/` and
`src/archutils/Win32/` (`RageSoundDriver_WDMKS.cpp` 5,
`ErrorStrings.cpp` 4, `MovieTexture_FFMpeg.cpp` 3,
`CrashHandlerNetworking.cpp` 2, `InputHandler_DirectInput.cpp` 2,
`VideoDriverInfo.cpp`/`USB.cpp`/`RegistryAccess.cpp`/`DialogUtil.cpp`/
`RageSoundDriver_Generic_Software.cpp`/`DSoundHelpers.cpp`/
`MemoryCardDriverThreaded_Windows.cpp`/`InputHandler_Win32_RTIO.cpp`/
`ArchHooks_Win32.cpp` 1 each -- same recurring narrowing patterns,
mostly Win32 API `int`/`DWORD`/`WORD` params fed `size_t`). One site
was in **vendored** code (`extern/ffmpeg-w32-prebuilt/include/
libavutil/common.h`'s inline clamp helpers, instantiated from
`MovieTexture_FFMpeg.cpp`) -- not patched (never edit vendored code);
instead scoped an MSVC-only `#pragma warning(push)` / `disable: 4244`
/ `pop` around just the FFmpeg `#include` block in
`MovieTexture_FFMpeg.h`, the same "leave vendored trees alone" policy
already used for the `ixwebsocket` clang-tidy exclusion (item 12).
**DONE 2026-09-11.** A genuinely edit-free, full-tree `--clean-first`
Debug rebuild with `/wd4244`/`/wd4267` removed now completes with
**exit code 0 and zero `C4244`/`C4267` warnings anywhere** --
`/wd4244`/`/wd4267` **removed from `src/CMakeLists.txt` permanently**,
completing the MSVC-warning ratchet (all 5 categories now promoted to
`-Werror`: `C4189`, `C4702`, `C4100`, `C4244`, `C4267`). Total sweep
across the whole session: real starting count was misjudged multiple
times before landing on the true baseline of **528 unique sites**
(`RageUtil.h`/`RageTimer.h`'s Release-target sites aside), fixed across
~140 files in ~16 verified batches, ending with the 26 late-discovered
`arch`/`archutils` sites bringing the true grand total past 550. Every
fix was a `static_cast`/casting-lambda documenting pre-existing
intentional narrowing (Win32 API params, Lua bindings, iterator diffs,
reverse-loop bounds, `RString::Left`/`Right` lengths) -- zero logic
changes; `sm_tests` held at 5966/226 throughout, and every §5-protected
site was re-verified against its characterization test.

### 3. Stale cppcheck leak list — DONE 2026-09-05, all dismissed
~~`Docs/Devdocs/possible memory leaks.txt` — from 2009. Re-triaged by
hand against the current tree (cppcheck itself isn't installed on the
maintainer box; each site was verified by reading the actual
ownership path instead). All 11 in-scope entries are either false
positives (cppcheck can't model this codebase's manual ownership
idioms — manager-owned resources, sound-reader chains, explicit
refcounting, Actor-tree AddChild/DeleteAllChildren, Lua-script-managed
lifetime, or a deliberate Meyer's-singleton) or already fixed since
2009 (`Font.cpp`'s `pPage`, `RageFileDriverDeflate.cpp`'s `mem` — the
latter via `std::unique_ptr`). `AdjustSync::s_pTimingDataOriginal` no
longer exists (refactored to a `std::vector` value member). Two
referenced files (`PitchDetectionTestUtil.cpp`, `crypto/CryptRSA.cpp`)
are gone entirely. Two entries are non-Windows paths, left unevaluated
per `AGENTS.md` §3 (`archutils/Unix/CrashHandlerChild.cpp`,
`smpackage/ZipArchive/Linux/ZipPlatform.cpp`). Full per-item reasoning
recorded in the file itself and in `log.md`.~~

---

## Tier 2 — Rot / dead weight (cheap, low-risk)

### 4. Dead CI committed — DONE 2026-09-03 (`e065f69c8b`)
~~`.travis.yml` (travis-ci.org shut 2021) + `.appveyor.yml` (VS2015,
v140_xp, IRC reporter, stale token). Removed. Live CI is
`.github/workflows/ci.yml`.~~

### 5. Dead IRC notifier — DONE 2026-09-03 (`718d3b3ec1`)
~~`src/CMakeProject-irc.cmake` + `src/irc/appveyor.cpp` — orphaned
(not `include()`d anywhere), targeted defunct `irc.freenode.net`.
Removed.~~

### 6. Stale build docs — DONE 2026-09-03
~~`Build/README.md` said "CMake min 2.8.12", "latest 3.3.0-rc3"; actual is
CMake 3.20 + C++17. Rewrote `Build/README.md` and `Build/INSTALL.md` to
current reality (`cmake -B build`, VS 2022, Windows 11 floor, no dev
install step, `StepMania-R` exe names). Fixed the matching 2.8.12 line and
`Build/StepMania.sln` path in `DocsAgents/build.md`. Root `README.md`
Travis badges were already removed.~~

### 22. Dead autotools build system — DONE 2026-09-10
The tree still carries a complete GNU autotools build: root `configure.ac`
(`AC_PREREQ(2.59)` — 2003; `AC_INIT(StepMania, 5.0, …, http://stepmania.com)`
— still identifies as *upstream*), `autogen.sh`, `autoconf/` aux dir, and
`Makefile.am` in the repo root + `src/` (+ each first-party subtree).
**It is dead:** CI (`.github/workflows/ci.yml`), `AGENTS.md`,
`DocsAgents/build.md` and the `Build/` docs are 100% CMake; nothing
regenerates or runs `configure`. `src/Makefile.am` still lists sources by
hand (drifts from `CMakeData-*.cmake` on every add) and its only unique
content was the `src/tests/` autotools test harness.
~~**Action:** delete `configure.ac`, `autogen.sh`, `autoconf/`, and
every first-party `Makefile.am`.~~ **Done 2026-09-10** — removed
`Makefile.am` (root), `src/Makefile.am`, `configure.ac`, `autogen.sh`,
`autoconf/` (`config.rpath` + `m4/*.m4`), `Utils/make-src-archive.sh`
(the `autoreconf -if` + `make dist` helper), and the autotools-output
block from `.gitignore` (`configure`, `aclocal.m4`, `config.status`,
`autom4te.cache`, `Makefile.in`, `src/config.h{,.in}`, `src/stamp-h1`,
…). ~3.9k lines. `extern/*/Makefile.am` kept (vendored). First done
`src/tests/` (`604af60680` prior, item 17 harnesses). **Verified:**
CMake reconfigure + `sm_tests` clean, suite 5839/220, `ctest` 100%;
Release `StepMania-R.exe` links clean under `WITH_WERROR=ON`;
`--SelfTest` exit 0. Nothing in CI / CMake / docs referenced any of it.
Left alone (not autotools-*system*, no live caller either — orphan
scripts, own cleanup if wanted): `Utils/CreatePackage.pl`,
`Utils/install-stepmania.pl`.

### 23. Dead `src/smpackage/` MFC tool — DONE 2026-09-10
`src/smpackage/` is the old standalone "SMPackage" installer/exporter
GUI: **85 `.cpp`/`.h`, ~19,000 LOC** of MFC (`CDialog`/`CTreeCtrl`/
`afxwin.h`) + a bundled `ZipArchive/` 3rd-party lib (`mfc/` + `stl/` +
`Linux/` variants, VS2003/VS2008 `.vcproj`, `borland.zip`), + `res/` +
`.rc`. ~1.1 MB.
**It is dead:** no CMake reference anywhere; no live `src/` code
includes any smpackage header or `SMPackageUtil`; `baseline.md`'s
clang-tidy sweep already excludes it as non-first-party; last real
commit message is *"I still can't get SMPackage to compile"*
(`7187efeab4`). MFC is not in a standard modern VS install, so it
cannot build on the maintainer's box regardless. The only feature it
provided — `.smzip` export — is separately dead
(`ScreenOptionsExportPackage`, see item 19).
~~**Action:** `git rm -r src/smpackage`.~~ **Done 2026-09-10** —
110 files / ~19k LOC removed. Verified: CMake `sm_tests` builds clean,
`[zip]` reader tests pass, Release `StepMania-R.exe` links clean under
`WITH_WERROR=ON`, `--SelfTest` exit 0. The `.smzip` **load** path
(`RageFileDriverZip` + `RageFileManager::Unzip`, both current-lib and
untouched) is unaffected. `Utils/make-src-archive.sh`'s
`./src/smpackage*` line goes with item 22 (that script is autotools
`make dist` flow).

### 24. Dead in-tree `src/libtomcrypt/` + `src/libtommath/` copies — DONE 2026-09-10
The fork moved libtomcrypt/libtommath to `extern/` git **submodules**
(`638ce07d84` "Add missing submodules"); the crypto targets are defined
in `extern/CMakeLists.txt` → `CMakeProject-tomcrypt.cmake` /
`-tommath.cmake`, sourced from `extern/libtomcrypt/src/...` with the
public include dir `extern/libtomcrypt/src/headers` (that's what
`CryptManager.cpp`'s `#include <tomcrypt.h>` resolves to). The old
**in-tree copies** `src/libtomcrypt/` + `src/libtommath/` (658 files,
~161k lines, ~10 MB; last touched pre-submodule by "Upgrade to
libtomcrypt 1.18.2") had **zero CMake reference** and their own dead
`libtomcrypt_VS2008.vcproj`.
~~**Action:** `git rm -r src/libtomcrypt src/libtommath`.~~ **Done** —
658 files / ~161k lines removed. **Verified:** CMake reconfigure clean,
`tomcrypt.lib` + `tommath.lib` still build from `extern/`,
`StepMania-R.exe` links clean (Release, `WITH_WERROR=ON`), `sm_tests`
5839/220, `--SelfTest` exit 0.

### 25. Misc orphan files — DONE 2026-09-10
Small dead files with no build role: `src/smpackage-net2008.vcproj`
(orphaned by item 23), `src/verify_signature/` (C++/C#/Java reference
impls of `.smzip` signature checking, not built, no reference),
`src/archutils/Win32/verinc.{c,exe,sln,vcproj}` (pre-CMake
version-increment tool + a **committed `.exe`**; CMake generates the
version stub from `src/verstub.in.cpp` via `StepmaniaCore.cmake:352`),
`CMake/VerStubUtil.cmake` (`configure_file`s a non-existent
`src/version_updater/verstub.cpp.in`; not `include()`d anywhere).
Left: `src/update_check/check_sm5.php` — server-side script,
`NetworkSyncManager.cpp` still references `/stepmania/check_sm5.php` as
the update endpoint; leave until the netcode's update-check is
revisited.
Also removed (`f13444d740`): stale committed
`src/archutils/Win32/mapconv.exe` (mapconv is CMake-built;
`*.exe` is already gitignored).

### 26. Committed binary blobs in the repo — audit / propose removal
`git ls-files` still tracks a pile of committed binaries outside
`extern/` and `Build/`. Each needs a "is this a real dep?" check before
removal — do NOT blanket-delete:
- **`Utils/Graphviz/`** (20 files: `dot.exe`, `neato.exe`, DLLs, `.lefty`
  scripts) and **`Utils/doxygen/{doxygen,hhc}.exe`** — **DELETED
  2026-09-11.** Confirmed dead first: `Docs/Doxyfile` has
  `HAVE_DOT = NO` (Graphviz call-graphs were never even wired on) and a
  stale `DOT_PATH` pointing at a Linux path from the original author's
  box, not the bundled Windows binaries. `Utils/doxygen_run.bat` updated
  to call `doxygen`/`hhc` bare (install your own, on PATH) instead of
  the bundled copies.
- **`Utils/{Bitmap Font Builder.exe, PngAlphaView.exe, forfiles.exe,
  pngcrush.exe, upx.exe, crush}`** — **DELETED 2026-09-11.** None
  referenced by any build/CI/packaging step (`forfiles.exe` duplicated
  a Windows built-in; the "call it from doxygen_run.bat" line was
  already `rem`-commented out). `Utils/pngcrushallfiles.bat` updated to
  note `pngcrush` must be on PATH now (its `SET PATH=...c:\stepmania\
  stepmania\utils` line was already a dead absolute path from someone's
  old machine).
- **`Program/parallel_lights_io.dll`** — **AUDITED 2026-09-11, KEPT.**
  `src/arch/Lights/LightsDriver_Win32Parallel.cpp` (compiled in,
  self-registers via `REGISTER_LIGHTS_DRIVER_CLASS`, user-selectable at
  runtime by name) does `LoadLibrary("parallel_lights_io.dll")` at
  runtime -- a real, working (if niche) dependency for anyone who
  selects the "Win32Parallel" lights driver with actual parallel-port
  lighting hardware. Not a build-time link dependency (no import lib),
  so its absence wouldn't break the build, but deleting it would
  silently break that driver at runtime for whoever still uses it.
  Leave it; removing it is really "deprecate the Win32Parallel lights
  driver," a separate, bigger decision.
- **`src/archutils/Win32/ddk/`** — checked 2026-09-10. The `.lib` files
  (`{x86,x64}/{dbghelp,hid,setupapi}.lib`) do appear dead: nothing in
  any `*.cmake`/`CMakeLists.txt` references `ddk`, and the Win32 build
  links `dbghelp`/`setupapi`/`hid` as bare names off the Windows SDK
  `LIBPATH` (`src/CMakeLists.txt:392-420`). BUT the **headers** in this
  dir are load-bearing: `src/archutils/Win32/USB.cpp` has hard-coded
  `#include "archutils/Win32/ddk/setupapi.h"` and `.../ddk/hidsdi.h`
  (the only include path is `src/`, so these resolve here, not to the
  SDK). So the directory must stay until USB.cpp is migrated to SDK
  headers — a Win32 USB/HID input behavior-risk change, not a sweep
  item. Removing only the 6 `.lib` files is safe but low value
  (~380 KB) and leaves the dir half-populated; deferred to a
  maintainer call. Left entirely as-is for now.
- **`Xcode/Libraries/*.a`** — committed macOS static libs. `AGENTS.md`
  §3 — leave to a macOS-focused pass.

---

## Tier 3 — Risk; deliberate decisions (see ADR 0001, ADR 0003)

### 19. `CreateZip` was STORED-only dead code — DELETED 2026-09-10
~~`tests/test_Zip.cpp` pins it: `CreateZip`/`TZip` (the bundled Info-ZIP
writer in `src/CreateZip.cpp`) emits `STORED` for every file — its
deflate path was ripped out (`FAIL_M("deflate removed")`).~~
Investigation showed **nothing live called `CreateZip`** — its only
would-be caller, `ScreenOptionsExportPackage`'s `ExportPackage()`, had
its whole body `#if 0`'d out for years ("XXX: totally doesn't work.
-aj", referencing a long-gone `RageFileObjZip` class) and always
returned false. Removed `src/CreateZip.{cpp,h}` (1126 lines, a 2009
SM4-beta Info-ZIP fork with a hand-rolled `crc32`) + its build-list
entries; gutted the dead `ExportPackage()` comment block to a clean
stub. `tests/test_Zip.cpp` was rebuilt to assemble its test archive
with a ~90-line in-file minimal ZIP writer (STORED + a DEFLATED entry
via `RageFileObjDeflate`), so it still fully characterizes
`RageFileDriverZip` (the reader) with no engine-side writer.
The `.smzip` *decompression* path is current: **zlib 1.3.2**
(`RageFileObjInflate`, the VFS mount) + **miniz 2.2.0 / MZ_VERSION
10.2.0** (`RageFileManager::Unzip`, the Lua-exposed bulk extractor).
If package *export* is ever wanted back: wire it through miniz's
`mz_zip_writer_*` (drop `#define MINIZ_NO_ARCHIVE_WRITING_APIS` in
`extern/miniz/miniz.h`) — miniz is already vendored + trusted for
reading.

### 21. `StringToInt` / `StringToLong` / `StringToLLong` deref `LOG` on the error path — FIXED 2026-09-09
~~`RageUtil.cpp:1886+` — the `std::sto*` `try` blocks caught
`invalid_argument`/`out_of_range` and called `LOG->Warn(...)` with no
null check on `LOG`, so any call with a non-numeric string before `LOG`
was constructed segfaulted (hit while writing `test_RageUtil.cpp`'s
`StringConversion::FromString<bool>("true")` case).~~ Fixed: all 6 catch
sites now `if( LOG ) LOG->Warn(...)`. Covered by the restored `"true"`
case in `test_RageUtil.cpp`.

---

## Tier 4 — Hotspots (where ongoing passes concentrate; not bugs)

### 9. God objects / oversized TUs
`ScreenEdit.cpp` 6596 · `GameManager.cpp` 3614 · `Player.cpp` 3567
(30 TODO/HACK) · `GameState.cpp` 3523 (~2,100 `GAMESTATE->` call sites) ·
`ScreenGameplay.cpp` 3381 · `NoteDataUtil.cpp` 3379 · `Profile.cpp` 2897.
**Action:** [`playbooks/split-god-object.md`](./playbooks/split-god-object.md),
one cluster per PR, always §4.

### 10. RString everywhere
`typedef StdString::CStdString RString` (`global.h:107`), 723 files /
~8,429 uses. Declared retirement goal (ADR 0001 Settled #5).
**Action:** [`playbooks/migrate-rstring.md`](./playbooks/migrate-rstring.md),
per subsystem, opportunistic.

### 11. Pre-C++11 threading / smart pointers
`RageThreads` predates `std::thread`/`std::mutex`;
`RageUtil_AutoPtr.h` ("TODO: replace with c++11 smart pointers");
`RageUtil_WorkerThread`, `BackgroundLoader`.
**Action:** after the safety net exists; `RageThreads` is load-bearing
and cross-platform — a dedicated ADR-scoped effort, not a casual pass.

### 12. Mechanical modernize-* debt
`modernize-use-nullptr`, `-use-override`, `-use-equals-default`,
`-use-bool-literals`, redundant void args, etc. across the tree.
**Action:** [`playbooks/clang-tidy-subsystem-pass.md`](./playbooks/clang-tidy-subsystem-pass.md),
one subsystem + one check family per PR; record in
[`baseline.md`](./baseline.md).
**Passes landed:** see the per-subsystem table in `baseline.md`. As of
2026-09-09 the four mechanical checks (`readability-container-size-empty`,
`modernize-use-override`, `modernize-use-nullptr`,
`bugprone-macro-parentheses`) are **clear across every non-platform
subsystem group outside the §5 parse/write path** (`rage`, `singletons`,
`actor`, `screen`, `file-types`, `globals`, `data`-non-§5). See
`baseline.md` → "Full-config re-sweep — 2026-09-09" for the exact
remainder.
**Still open, by category (updated 2026-09-09 after the arch/Win32 pass):**
- §5-protected files (`Song*`/`Steps*`/`NotesLoader*`/`NotesWriter*`/
  `TimingData`/`.crs`) — ~95 hits, blocked on a parse/course
  regression corpus.
- vendored `ixwebsocket` subtree (`src/IX*.cpp`) — ~13 hits, don't
  touch vendored code.
- `modernize-use-equals-default` (38) and `readability-redundant-member-init`
  (28) — deferred: `--fix` output is too dirty to land without a
  coupled `clang-format` run, which ADR 0002 says must be its own
  change. `modernize-use-bool-literals` — DONE 2026-09-09
  (`3be07f669d`, closeout `3479323da9`), the diff was clean enough to
  land on its own.
- `bugprone-integer-division` (14) — flagged for the maintainer
  (sub-pixel render maths on untested paths), see `baseline.md`.
- 4 unfixable `bugprone-macro-parentheses` sites (`StatsManager` ×2
  `::`-scoped, `OptionRowHandler` MAKE(type), `Profile` LOAD_NODE(X)).
**Done:** `arch/` + `archutils/Win32/` driver code is now cleared for
`container-size-empty` / `use-override` / `use-nullptr` /
`macro-parentheses` (Windows-only TUs — `AGENTS.md` §3 allows Windows
platform work). The four mechanical checks are clear across all of
`src/` outside the two blocked buckets above.

### 13. `src/archutils/Win32/arch_setup.h` legacy — mostly DONE
- `isnan`/`isfinite` macros removed 2026-09-03 (`37e6766d5e`).
- `_WIN32_WINNT 0x0601` → `0x0A00`, `_WIN32_IE 0x0400` → `0x0A00`,
  `#define __STDC__ 0` removed, Win98/ME comment dropped — 2026-09-03
  (`5565039bf7`). Clean rebuild; runtime not yet smoke-tested.
**Done (2026-09-05, `a26c13e00c`):** dropped `_CRT_SECURE_NO_DEPRECATE`
(redundant with the CMake-level `_CRT_SECURE_NO_WARNINGS`) and
`_SCL_SECURE_NO_DEPRECATE` (no-op since VS2017 removed the checked-
iterator feature it suppressed), plus the stale ~30-line VC6/VC2005
comment block. Kept `_CRT_NONSTDC_NO_WARNINGS` (still functionally
relevant — POSIX-name deprecation). `arch_setup.h` itself is now clean.
**Remaining (speculative, no current failure to verify against):**
`src/archutils/Win32/DirectXErrorList.h` — 12 `case` labels
(`0x8007xxxx`) that don't fit signed `HRESULT`; MSVC compiles it fine
(the only compiler actually used on Windows today), clang would
reject it (C++11 narrowing) if clang-cl were ever adopted. Not touched
— no reproducer to verify a fix against, and Windows isn't built with
clang currently. Rewrite the cases as hex literals / `HRESULT(...)` if
and when clang-cl support is actually pursued.

### 17. Pick a unit-test framework + write core characterization tests — phases 1-3 DONE; phase 4 DONE for every format (.sm/.ssc + .pms/BMS + .dwi + .ksf + .sma + .crs); reader-salvage DONE (incl. file_errors)
Framework decided: **Catch2 v3** (amalgamated, vendored `extern/Catch2/`
@ v3.16.0) — ADR [0006](./adr/0006-test-harness.md). Build approach:
`src/` → OBJECT library `sm_engine`, shared by the exe and a new
`sm_tests` target, behind `WITH_TESTS` (default OFF, CI-on).
**Scaffold merged to `5_1-new` (2026-09-04):** `src/CMakeLists.txt`
OBJECT-library split, `tests/CMakeLists.txt`, `tests/test_RageUtil.cpp`
(first characterization coverage), Apple `SMMain.mm` entry-point split,
CI jobs for Windows/macOS/Linux × `sm_tests`. All 8 CI jobs green; local
Windows Release build + `--SelfTest` also verified (§4 gate). Fixed en
route: `LoadingWindowGtk` OBJECT → STATIC (Linux-only link fix, see
`log.md` 2026-09-04).
**Progress (ADR 0006 phases 2-4):** `RageMath` done (2026-09-04,
`f98d7a489e`, wave/matrix/vector/bezier helpers). `TimingData` done
(2026-09-05, `197ea46f02`) — row/measure math and beat<->time
conversion via the `NoOffset` entry points (the offset-applying
wrappers just add a `GAMESTATE`/`PREFSMAN` read on top, so this
covers the real logic without needing a live engine). `NoteData` done
(2026-09-05, `6d4e6b5aa0`) — tap/hold storage, track queries, row
traversal; its counting/statistics API (`GetNumTapNotes`,
`GetNumMines`, `GetNumHoldNotes`, `GetNumRowsWithSimultaneousTaps`,
...) calls `GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow()`
through `IsTap`/`IsMine`/`IsLift`/`IsFake` and is out of scope for a
GAMESTATE-free test file — only `GetNumTapNotesNoTiming` (the
GAMESTATE-free counterpart) is covered. `NoteDataUtil` done
(2026-09-05, `46001925f5`) — the transform helpers that operate
purely on `NoteData` (and, for `RemoveFakes`, a caller-supplied
`TimingData const&` instead of the global timing data): hold/roll
sub-type conversion, `RemoveSimultaneousNotes`
(`RemoveJumps`/`RemoveHands`) including its removal-order quirk (the
*last* pressed track survives a cutdown, not the first) and that held
tracks count toward the threshold but are never themselves removed,
type filtering (`RemoveMines`/`RemoveLifts`/`RemoveAllTapsOfType`/
`RemoveAllTapsExceptForType`), `ShiftLeft`/`ShiftRight`'s wrap-around
track rotation, `InsertRows`/`DeleteRows`, and
`GetNextEditorPosition`/`GetPrevEditorPosition` treating a hold's tail
as its own stop distinct from its head. 246 assertions / 52 cases
total. `NotesLoader*` done (2026-09-05, `69803e8ca1`,
`tests/test_NotesLoader.cpp`) — the parse *primitives* that turn
simfile text into beats/rows/`TimingData`: the `MsdFile` tokenizer
(`:` param breaks, missing-`;` recovery at the next line's `#`, `//`
comments, `\:` escape under `bUnescape`), `GetMainAndSubTitlesFrom
FullTitle` (five separators, tab-before-`" -"` precedence, the
separator's non-space half stays in the subtitle),
`SMLoader::RowToBeat` (`r`/`R` suffix → ÷rowsPerBeat), `ParseBPMs`/
`ParseStops` (split `,` then `=`, row-suffix conversion — valid input
only), `ProcessBPMsAndStops` (initial BPM at row 0, pre-beat-0 stop →
song offset not segment), `ProcessDelays`/`ProcessTimeSignatures`
(implicit-4/4 back-fill)/`ProcessTickcounts` (clamp to
`ROWS_PER_BEAT`). **Scoped out, deliberately:** full `LoadFromDir`/
`LoadFromSimfile` needs live `FILEMAN`+`LUA`+more → `--SelfTest` smoke
territory per the playbook; the `SMLoader` helpers' error/edge
branches (`"a=b=c"`, zero BPM, zero-length stop, negative beat) all
call `LOG->UserLog()` and `LOG` is null in the harness. 311 assertions
/ 72 cases total. **ADR 0006 phase 2 (pure-ish core characterization)
is complete.**

**Bootstrap fixture DONE (2026-09-06, `tests/EngineTestEnv.{h,cpp}`).**
`EngineTestEnv::Require()` news up `LUA` → `FILEMAN` → `LOG` → `GAMEMAN`
once per `sm_tests` run and mounts `tests/data/` at `/testdata`; a
Catch2 listener tears it down. Retires the "`LOG` is null" scope-out
above.

**Phase 4 for `.sm`/`.ssc` DONE (2026-09-06,
`tests/test_NotesLoaderCorpus.cpp`).** A `GENERATE(from_range(...))`
parse-regression over the **real committed SM5 sample songs**
(`Songs/StepMania 5/{Goin' Under (.sm + .ssc), MechaTribe Assault,
Springtime}`) — **no toy simfiles** (maintainer call 2026-09-06: for
simfiles always use the real ones; a toy only proves the loader
survives input its author understood, not the §5 invariant — see
`tests/data/README.md`). Pins per-song metadata + BPM and per chart
(file order, 39 charts) `StepsType`/`StepsTypeStr`/difficulty/meter/
track count/`GetNumTapNotesNoTiming`, characterization values captured
from a hidden `[.dump]` case. Plus a Goin' Under `.sm`-vs-`.ssc`
cross-format equivalence case. `#NOTES` parsing works now that
`GAMEMAN` is in the fixture; `EngineTestEnv` also mounts the repo
`Songs/` at `/Songs`. Suite **638 assertions / 78 cases** (was
311/72). See `log.md` 2026-09-06 and ADR 0006 phases 3-4.

**`PREFSMAN` added to `EngineTestEnv` (2026-09-06).** Construction is
now `LUA → FILEMAN → LOG → PREFSMAN → GAMEMAN` (teardown reversed —
`~PrefsManager` calls `LUA->UnsetGlobal`). No `.ini` is mounted so every
preference keeps its compiled default; `tests/test_EngineTestEnv.cpp`
pins the bring-up + a few defaults (`m_bQuirksMode` false, `m_bFastLoad`
true, `m_fGlobalOffsetSeconds` -0.008). Suite **651 / 81**. This
unblocks the `LoadFromDir` path for `.dwi`/`.ksf`/`.bms`.

**`.pms` / BMS family DONE (2026-09-06, `tests/test_NotesLoaderBMS.cpp`
+ `tests/data/pms-fixture/`).** `BMSLoader::GetApplicableFiles` +
`LoadFromDir` over a **derived** 3-chart `.pms` set: real note/timing/
`#WAVxx`-channel structure byte-for-byte, but title/artist/genre and
`#WAV` filenames scrubbed to placeholders and keysound audio replaced
with 44-byte silent stub WAVs (the parser only `IsAFile`s them). The
real Pop'n Music song is Konami's and not redistributable; the derived
approach keeps the §5 regression honest without shipping copyrighted
content (`tests/data/README.md`). Pins title/artist, BPM,
`m_vsKeysoundFile` size (54), and per chart type/difficulty/meter/
tracks/taps for `pnm-five` / `bm-double7` / `pnm-nine`. Suite
**676 / 82**.

**`.dwi` DONE (2026-09-06, `tests/test_NotesLoaderDWI.cpp` +
`tests/data/dwi-fixture/`).** Same derived-fixture pattern:
`DWILoader::GetApplicableFiles` + `LoadFromDir` over a real 3-chart
`.dwi` with only `#FILE`/`#TITLE`/`#ARTIST` scrubbed (diff-verified;
DWI has no keysounds so nothing else to stub). Pins the
`GetMainAndSubTitlesFromFullTitle` `" ("` split, `#GAP`→offset
(-0.065), `#BPM` 145 + `#CHANGEBPM` 72.5 @ beat 256, `#SAMPLESTART`
79.485, and 3 `dance-single` charts (`BASIC`/`ANOTHER`/`MANIAC` →
Easy/3/233, Medium/8/443, Hard/10/680 taps). Scrub verified lossless
against the source. Suite **705 / 83**.

**`.ksf` DONE (2026-09-08, `tests/test_NotesLoaderKSF.cpp` +
`tests/data/Fixture Artist - KSF Fixture/`).** Same derived-fixture
pattern over a 4-chart Pump It Up KSF set — only `#TITLE`/`#ARTIST`/
`#STEPMAKER`/`#SONGFILE` scrubbed, everything else byte-for-byte
(KSF has no keysounds). Quirks the fixture had to respect: KSFLoader
needs `song.SetSongDir()` (no `Dirname` fallback), derives type +
difficulty from the *filename* (`"double"` → pump_double + Medium; no
keyword → pump_single + Hard), and derives the artist from the
*directory name* split on `" - "` (it ignores `#ARTIST`). Pins `#BPM`
220, `#STARTTIME`→offset -0.17, and 4 charts (pump-double Medium/17 &
26, pump-single Hard/17 & 23; 601/895/622/807 taps). New StepsType
coverage: `pump-double`. Verified identical to the untouched source
folder. Suite **948 / 118**.

**Still open:**
- `.sma` **DONE 2026-09-10** (`test_NotesLoaderSMA.cpp` +
  `tests/data/sma-fixture/`). No real `.sma` exists anywhere (extinct
  format); maintainer searched, found none, ruled the current read
  behavior the reference. The fixture is **synthetic** (not derived —
  nothing to derive from), hand-authored to pin the SMA-only tags
  (`#ROWSPERBEAT` row translation, `#BEATSPERMEASURE`, `#SPEED`
  seconds-unit, `#MULTIPLIER`) and the song-vs-Steps timing split.
  Latent-bug note (NOT fixed, §5): `SMALoader`'s `#ROWSPERBEAT` handler
  indexes `split(expr,"=")[1]` without a size check, so a malformed
  `#ROWSPERBEAT:4;` (no `=`) is an OOB read → crash. Left as-is per the
  "current behavior is the reference" decision; flagged for the
  maintainer.
- `.crs` **DONE 2026-09-10** (`test_NotesLoaderCRS.cpp`). Uses
  `CourseLoaderCRS::LoadFromBuffer` over inline course text — no fixture
  file, no SONGINDEX (the `bFromCache=true` path skips the cache probe).
  Pins metadata, `#SONG` resolution vs an empty `SONGMAN`, difficulty /
  meter-range parsing, modifier keywords. Findings:
  - **`#STYLE` was dead code — FIXED 2026-09-10.** `LoadFromMsd`'s
    recognised-tag guard was
    `else if( !eq("DISPLAYCOURSE") || !eq("COMBO") || !eq("COMBOMODE") )`
    — always true (no name equals all three), so `#STYLE`, the
    RADAR-cache branch and the "unexpected value" log after it were
    unreachable and `#STYLE` on a course did nothing. Changed to
    `eq(A) || eq(B) || eq(C)` and moved the `#STYLE` handler above the
    `bFromCache` catch-all (otherwise a buffer/cache load routes
    `#STYLE` into the radar-cache parse). Test now checks `#STYLE`
    populates `m_setStyles`. §5 change — Windows-verified, and the test
    is the old-vs-new record.
  - **2-part `#SONG:Group/Song` refs crash headlessly** (not fixed — not
    a real-engine bug). `SONGMAN->FindSong(group, song)` →
    `GetSongs(group)` → a `FOREACH_EnabledPlayer` loop that dereferences
    `PROFILEMAN` (null in the fixture). `PROFILEMAN` always exists by
    course-load time in the real engine; the test sticks to 1-part title
    refs (resolve via `GROUP_ALL`).
- **Headless theme metrics — DONE 2026-09-10.** `EngineTestEnv` now
  loads `SMRTest`, a scripts-free minimal theme
  (`tests/data/test-theme/`, `FallbackTheme=`, no `Scripts/`). See
  item 27 for the full story (diagnosed in a Docker container with
  gdb). `THEME->IsThemeLoaded()` is now true, `ThemeMetric` reads
  resolve, and `SONGMAN` is back in the fixture. `PlayerOptions` /
  `RadarValues` / theme-metric tests are no longer blocked on this —
  add them when convenient.
- `src/tests/test_file_readers.cpp` **DONE (2026-09-06, extended
  2026-09-09)** → `tests/test_RageFile.cpp`: `RageFile` open/read/write/
  seek/tell/`GetLine`/`AtEOF` through `FILEMAN`'s `/@mem` writable
  mount, so each test writes its own file — no committed fixtures.
  Pinned the stdio-like `AtEOF` semantics (`m_bEOF` only trips on a
  0-byte read), `Seek`-past-end clamping, `GetLine` behaviour. 67
  assertions / 8 cases.
  Plus `tests/test_RageFileDeflate.cpp` (2026-09-09) — the salvage of
  `TestDeflate()`: `RageFileObjDeflate`/`RageFileObjInflate` raw-deflate
  round-trip over `RageFileObjMem` (byte-exact + CRC32 in==out, several
  read/write block sizes, `RageFileObjInflate::Seek`). Also pins two
  gotchas found doing it: `RageFileObjDeflate::FlushInternal` emits
  `Z_FINISH` (ends the stream) and the dtor calls it too — never call
  `Flush()` yourself; and `RageFileObj::Read(RString&,int)` trims to the
  count read but does **not** clear the buffer first, so a reused
  `RString` returns stale bytes. 7 cases.
- `src/tests/test_audio_readers.cpp` **DONE (2026-09-06)** →
  `tests/test_RageSoundReader.cpp`: the WAV decoder, from a synthetic
  PCM WAV built in the test (deterministic samples — no copyrighted
  audio, no committed fixture) written to `/@mem` and decoded back.
  Both `RageSoundReader_WAV::Open` and the `OpenFile` autodetect
  factory (which needed `ActorUtil::InitFileTypeLists()` added to
  `EngineTestEnv`). Pinned sample rate / channels / `GetLength` (ms) /
  PCM16→float / `SetPosition`. 27 assertions / 3 cases.
- `src/tests/test_file_errors.cpp` **DONE (2026-09-09)** →
  `tests/test_RageFileErrors.cpp`: a small in-test VFS driver
  ("ERRTEST", a modern-interface port of the 2004 `RageFileDriverTest`
  — `FilenameDB.AddFile` + a self-registering `FileDriverEntry` +
  `RageFileObj` subclass) serves one file from a `std::string` and
  fails the read/write/flush that crosses a byte threshold. Pins that
  a mid-stream driver error propagates as `Read`/`Write` → -1 with
  `GetError()=="Fake error"`, that `Flush()` surfaces `FlushInternal`'s
  error, and that `IniFile::ReadFile`/`WriteFile` return false + carry
  the error up. 6 cases.
  **Reader salvage complete** — all three old file/audio test files are
  now covered. Still in `src/tests/`: `test_vector.cpp` (macOS/altivec,
  §3 — skip), `test_threads.cpp` (`RageThreads`, backlog item 11 —
  ADR-scoped).

**Pure-core coverage keeps growing (2026-09-06):**
- `tests/test_IniFile.cpp` — `IniFile` (`.ini` reader/writer under
  `Preferences.ini` / keymaps / `Static.ini` / theme-metrics fallback /
  the `[Char Widths]`→`[main]` fixup), previously untested. 76
  assertions / 11 cases, strings via `/@mem`. Pinned the parse quirks
  (trimmed key vs untrimmed value, comment prefixes, pre-section drop,
  `\`-continuation, `LOG->Warn` on missing `=`) +
  `RenameKey`/`DeleteKey`/round-trip.
- `tests/test_XmlFile.cpp` — the hand-rolled XML parser
  (`XmlFileUtil::Load`/`GetXML`), previously untested. 43 assertions /
  12 cases, from strings. Pinned: node-is-root, text-only-before-first
  -child, five-named-entities-only (no numeric refs), unquoted +
  name-only attrs, prolog/comment skipping, `GetXML` round-trip.
Suite **918 / 117**.

### 16. Pre-floor `#if` guards across `src/arch/` and `src/archutils/` — Windows runtime-version checks DONE
Now that ADR 0003 sets Windows 11 / current-macOS / current-Linux floors,
sweep for `#if`/`#ifdef` guards handling below-floor OSes: `_WIN32_WINNT`
comparisons, `WINVER` checks, `MAC_OS_X_VERSION_MIN_REQUIRED` for old
10.x, XP/9x fallback branches, 32-bit paths, EOL-distro `#ifdef`s.
**Done (2026-09-04), Windows runtime-version checks:**
`ArchHooks_Win32.cpp::BoostPriority()` ran `IsWindowsVersionOrGreater
(Win2000)` at runtime to decide whether `ABOVE_NORMAL_PRIORITY_CLASS`
was usable (always true on the floor) — collapsed to unconditional.
`DSoundHelpers.cpp` did the same for `IsWindowsVistaOrGreater()` gating
`DSBCAPS_TRUEPLAYPOSITION`. Both now-unused `#include "VersionHelpers.h"`
removed (`DirectXHelpers.h` and the two call sites). No preprocessor-level
`#if _WIN32_WINNT`/`WINVER` guards remain in `src/arch`/`src/archutils`
outside `arch_setup.h` (already correct at `0x0A00`, item 13). A few
comments mention Win9x/XP/Win98 as historical context on still-live code
(hardware quirk notes, page-permission differences) — left alone, not
functional gates.
**Not done — 32-bit Windows removal (new, see item 21).**
**Remaining:** macOS `MAC_OS_X_VERSION_MIN_REQUIRED` / old-`.mm` sweep,
Linux EOL-distro `#ifdef`s — both out of scope for now (`AGENTS.md` §3,
non-Windows work needs explicit instruction).

### 21. Drop 32-bit Windows (x86) — DONE 2026-09-11
ADR [0003](./adr/0003-platform-support-floors.md) (Accepted): "No 32-bit
targets on any platform." Windows 11 (the floor) doesn't even ship a
32-bit edition, so a Windows-R x86 build has nowhere to run — but the
x86 build path was still fully present. Maintainer approved executing
it. Changes:
- `StepmaniaCore.cmake`: `SM_WIN32_ARCH` detection replaced with
  `message(FATAL_ERROR ...)` on a non-64-bit `CMAKE_SIZEOF_VOID_P`
  (loud rejection instead of silently mislabeling), then unconditional
  `set(SM_WIN32_ARCH "x64")`.
- `src/CMakeLists.txt`: the `if(SM_WIN32_ARCH MATCHES "x86") /arch:SSE2`
  branch removed (dead now — SSE2 is baseline on x64, that flag only
  ever mattered for 32-bit MSVC).
- `extern/CMakeProject-mad.cmake`: the `if(SM_WIN32_ARCH MATCHES "x64")
  FPM_64BIT else() FPM_INTEL` branch collapsed to the `FPM_64BIT`-only
  path (`FPM_INTEL`, libmad's 32-bit fixed-point mode, was dead).
- `tests/CMakeLists.txt` / `CMake/Modules/FindDirectX.cmake`: no change
  needed — they only *use* `${SM_WIN32_ARCH}` as a path component, so
  they resolve to `x64` automatically now.
- `build-ffmpeg-win32.yml`: dropped the `i686-w64-mingw32` toolchain
  install, the whole "Build x86" step, and the x86 half of packaging —
  x64-only now. Manual-trigger workflow (item 7), doesn't touch regular
  CI; the already-cut `x64/` Release asset stays valid.
- Left alone (out of scope — these are cross-platform CPU-family
  detection, not "32-bit Windows"): the `CMAKE_SYSTEM_PROCESSOR MATCHES
  "x86"` branches in `src/CMakeLists.txt` for the Linux backtrace method
  and the general `CPU_X86`/`CPU_X86_64` compile defines; `extern/
  libpng/CMakeLists.txt`'s own arch check (vendored).
Verified: Windows Release build + `sm_tests` (Debug) + `ctest` +
`--SelfTest` all green after the change (`-A x64` explicit).

### 28. Drop macOS x86_64 from CI — DONE 2026-09-11
ADR [0003](./adr/0003-platform-support-floors.md)'s macOS floor row
carried x86_64 conditionally: "arm64 primary, x86_64 while Apple/
Rosetta still ship it." Maintainer confirmed live (2026-09-11) that
Apple has ended Intel Mac support, so that clause has lapsed — same
kind of "the floor moved out from under this build" reasoning as item
21's 32-bit Windows drop. Changes:
- `.github/workflows/ci.yml`: removed the `macos-build-x86_64` job
  (`macos-15-intel` runner, `-DCMAKE_OSX_ARCHITECTURES=x86_64`).
  `macos-build-arm64` was already a separate job — it's now the only
  macOS build leg. `macos-tests` (unit tests) was already arm64-only,
  no change needed there.
- ADR 0003's floor table updated: macOS is now **arm64 only**.
- **Left alone, out of scope for this pass:** `CMake/CPackSetup.cmake`
  and `CMake/SetupFfmpeg.cmake` both still branch on
  `CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64"` (DMG packaging label
  "macOS-Intel", FFmpeg `--arch=x86_64` configure flags) — a local
  Intel-Mac dev build still works, only CI *coverage* changed here.
  Removing the x86_64 packaging/build path entirely (making arm64 the
  only buildable target, not just the only CI-tested one) is a bigger
  change and needs its own explicit go-ahead.
Verified: this is a CI-workflow + docs-only change (no C++ touched);
the push that carries it is itself the verification that the trimmed
`ci.yml` still parses and all remaining jobs (Windows/Ubuntu/macOS
arm64 × build+tests, Lua.xml validation) run green.

### 15. `#if 0` dead blocks — two batches DONE, ~7 remain (fragile ones)
First pass (`a2c3d44522`, 2026-09-04): 10 dead blocks across 8 files
(`CodeDetector.cpp`/`.h`, `CourseUtil.cpp`, `NoteDataUtil.cpp` ×3,
`NoteDataWithScoring.cpp`, `NotesWriterSSC.cpp`,
`RageSoundReader_Resample_Good.cpp` ×3, `RageUtil_FileDB.cpp`).
Second pass (`f1d6e6c4ac`, 2026-09-04): `RageUtil_AutoPtr.h` — three
blocks marked `#if 0 // broken VC6` weren't dead code at all, but
working `HiddenPtr<T>` cross-type conversion + the friend declaration
it needs, blocked by a VC6 template bug; **enabled**, not deleted (the
toolchain floor is MSVC v143, nothing left to work around).
`ScreenNameEntry.cpp` — two blocks explicitly labeled "DEBUGGING
STUFF"/"Debugging." (dead ad-hoc dev shortcuts). `StdString.h` — a
generic template superseded by the concrete overloads right after it.
**Left alone both passes, real kept-for-reference or ambiguous cases:**
`NoteData.cpp` (explains why the disabled iterator op is unsafe),
`RageFileManager_ReadAhead.cpp` (explains why `dup()` doesn't work
here), `RageDisplay_GLES2.cpp` (an active `#if 0`/`#else` selector —
the `#else` branch is what's actually compiled), `RandomSample.cpp` /
`ScoreKeeperNormal.cpp` (same, `#if 0`/`#else` or `#if 0`/`#elif 1`/
`#else`), `RageUtil_CachedObject.cpp` (a deliberately-disabled usage
example), `RageSoundReader_MP3.cpp`'s `resync()` (declared in the
header; a comment elsewhere explicitly says "don't use resync(), it's
slow" — deliberate non-use, not abandonment), `RageThreads.cpp`'s
mutex lock-order checker (a complete, working deadlock-detection
feature, just not wired in), `Player.cpp` (author's own "doesn't make
sense" comment, but it's CPU/autoplay scoring — too fragile a hot path
to guess at) and `ScreenEdit.cpp` (looks superseded by the logic that
replaced it, but no explicit disowning comment and it's fragile editor
state-machine code).
**Remaining (~7 sites):** `Player.cpp`, `ScreenEdit.cpp`, plus
whatever's left after re-verifying the rest weren't miscounted. Both
need someone to actually reason through gameplay/editor logic, not a
mechanical read. Out of scope: `archutils/Unix/*` and
`arch/Threads/Threads_Pthreads.*` (non-Windows, `AGENTS.md` §3),
`src/tests/` (unsalvageable, ADR 0006).
**Action:** locate the actual matching `#endif` before judging a
block — a mid-block partial read can look like it ends earlier than
it does (bit us once on `CourseUtil.cpp`, caught by the `WITH_WERROR`
build, not by inspection). Distinguish three shapes before deciding:
plain dead `#if 0 ... #endif` (candidate for removal), an active
`#if 0/#else` or `#if 0/#elif N/#else` selector (never remove — the
other branch is live), and a toolchain-EOL block like the VC6 ones
above (enable, don't delete).

### 18. Logging overhaul — phases 1-3 DONE, phase 4 IN PROGRESS (methodology correction below)
Phase 1 (`c82d0e9058`): bracketed level tags, `Error()` level, no
`/////`, `Char Widths` fixed. `--SelfTest` log 695→467 lines, clean.
**Phase 2 DONE (2026-09-08, `156c075ff3` + `3a53baad5f`):**
`RageLog::Debug()`; `enum RageLog::LogLevel` (Trace<…<Error<Off) global
minimum filter; `namespace Log { enum Category }` seed taxonomy +
`CategoryFrom/ToString`; `LOG_TRACE/DEBUG/INFO/WARN/ERROR(cat, ...)`
macros stamping `<cat> file:line`; per-category minimum
(`SetCategoryLevel`/`GetEffectiveLevel`, a category can go below the
global); `SetLogLevelSpec` parses `--LogLevel=warn,gl:off,font:trace`
(pref `PrefsManager::m_sLogLevel`, `--LogLevel` override in
`ApplyLogPreferences`). `Write()` refactored to explicit
`(LogLevel, Log::Category)`. `test_RageLog.cpp` covers it.
**Phase 3 DONE (2026-09-08, `65fbca7bf5`):** `RageLog::Write` collapses
runs of identical consecutive lines — first two verbatim, 3rd+ folded
into `[TRACE] (previous line repeated N more times)` at the run's end.
`--SelfTest` `log.txt` 460→438, the biggest note eating 16 `glTexImage2D`
repeats. (Also fixed: `SetLogLevelSpec` resets before applying — the
spec is the whole config.)
**Remaining (ADR [0005](./adr/0005-logging-overhaul.md)):**
- Ph4: call-site audit per subsystem — bare `LOG->Trace/Warn/…` →
  `LOG_*` + a real `Log::Category`; `Warn`→`Trace` (expected fallback)
  / `Warn`→`Error` (real failure); this is what makes the per-category
  filter and the file:line column actually do anything, and where
  `LOG->Debug()` call sites first land. Long tail, per subsystem.
  **Ph4 batch 1 (2026-09-11, `572fc79738`):** first files migrated —
  `RageFileManager.cpp` (11 sites, `Log::File`), `IniFile.cpp` (11
  sites, `Log::File`), `ThemeManager.cpp` (9 of 12 sites, `Log::Theme`
  — its 3 `LOG->UserLog(...)` calls are a separate user.txt-facing
  facility with no `LOG_*` equivalent, left untouched/out of scope).
  Triage: genuine I/O/config failures → `LOG_ERROR`; routine/expected
  fallbacks (probe-style reads, "not found" on an already-optional
  delete) and security-guard "overwrite not allowed" sites → kept at
  `LOG_WARN`/`LOG_TRACE` rather than force-upgraded. No parsing/logic
  changed — pure category/level tagging. None of these 3 files are
  §5-protected. Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%,
  Release `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  **Ph4 batch 2 (2026-09-11):** `CryptManager.cpp` (21 sites,
  `Log::General` — no crypto-specific category exists, and `General`
  is explicitly "no subsystem / not yet categorised") and
  `MemoryCardManager.cpp` (13 real sites, `Log::Profile` — this
  subsystem exists to serve `PROFILEMAN`'s removable-media storage;
  one grep hit at line 219 is a pre-existing commented-out
  `//LOG->Trace("update")`, left untouched, not a real call site).
  Triage highlights: `CryptManager`'s RSA/hash/file-I/O failures
  upgraded to `LOG_ERROR`; the one-time "keys missing, generating new
  keys" first-run notice downgraded to `LOG_INFO` (routine, not a
  failure); the alternate-public-key "signature mismatch" site
  downgraded to `LOG_TRACE` (that function is called once per
  candidate key while probing for the right one, so most mismatches
  are expected, not tampering) to match the pre-existing "trying
  alternate key" `Trace` right above it. `MemoryCardManager`'s
  "mount failed" upgraded `Trace`→`WARN` (a real, if hotplug-flaky,
  operation failure) and its post-mount "GetFileDriver failed"
  upgraded `Warn`→`ERROR` (an internal inconsistency — the driver we
  just mounted can't be found); its ~10 routine device-tracking/
  thread-state `Trace` sites kept at `Trace`, just re-categorized.
  No parsing/behavior logic changed anywhere. Verified: `sm_tests`
  5966/226 unchanged, `ctest` 100%, Release `StepMania-R.exe` clean
  rebuild, `--SelfTest` exit 0.
  **Ph4 batch 3 (2026-09-11):** `RageSound.cpp` (12 real sites,
  `Log::Sound`; 4 grep hits are pre-existing commented-out `Trace`
  calls, left untouched) and `NetworkSyncManager.cpp` (14 real sites,
  `Log::Net`; 1 grep hit is a pre-existing commented-out call). Triage
  highlights in `RageSound.cpp`: the four "sound not loaded" API-misuse
  guards (`Play`/`Pause`/`GetLengthSeconds`/`SetPositionFrames` called
  before `Load()`) upgraded `Warn`→`ERROR` (a real caller bug, not
  routine); the missing/corrupt-file open failure in `Load()` upgraded
  to `ERROR` (falls back to a silence reader, but the missing asset is
  still a real problem); "seeked past EOF" and "invalid stop mode"
  (malformed theme/Lua input) kept at `WARN` (notable but non-fatal,
  self-clamps/no-ops); the start-time-in-the-past diagnostic upgraded
  `Trace`→`WARN` per its own comment ("log it, since it can be
  unobvious"). In `NetworkSyncManager.cpp`: "invalid port" and "failed
  to connect" upgraded to `ERROR` (abort the connection attempt); an
  out-of-range command byte from the wire upgraded `Trace`→`WARN`
  (a real protocol anomaly worth surfacing); the rest (connection-flow
  `Info`, per-packet/per-frame `Trace` chatter) re-categorized only.
  **Discovered mid-batch: `NetworkSyncManager.cpp` (and its whole
  calling subsystem — every `ScreenNet*`/`Room*` file) is not part of
  the CMake build at all** — absent from every `CMakeData-*.cmake`
  list, no object file in a from-scratch build. The edit is harmless
  (pure text, and the compiler never sees the file either way) but
  couldn't be verified by the usual `WITH_WERROR` compile gate; flagged
  as new backlog item 29 for a maintainer call (re-wire it into the
  build vs. remove it as confirmed-dead code), rather than
  investigated further in this batch. `RageSound.cpp` compiled and
  verified normally.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  **Ph4 batch 4 (2026-09-11):** `RageDisplay_OGL.cpp` (25 real sites,
  `Log::Gl`; 5 grep hits are pre-existing dead code — 2 fully
  commented-out lines plus 3 more sitting inside `/* ... */` blocks
  around debug-only matrix dumps — all left untouched). Triage
  highlights: shader file-open/read failures and actual GLSL
  compile/link failures upgraded to `LOG_ERROR` (real bugs — a shipped
  shader that won't build); driver-capability gaps ("fragment shaders
  not supported", "low-performance renderer") kept at `LOG_WARN`
  (expected on older/limited hardware, handled gracefully, not a
  bug); the vendor/renderer/version/extension-list startup dump and
  the various feature-probe fallback notices (paletted textures,
  packed-pixel format, pixel-map table size) kept at `LOG_INFO`/
  `LOG_TRACE` as before — all routine capability detection, not
  problems. The two known-driver-quirk workarounds (an old Catalyst
  `GL_INVALID_OPERATION` bug) stayed at `LOG_TRACE`, matching the
  file's own comment explaining why the fallback exists. No
  parsing/behavior logic changed; not §5-protected.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  **Ph4 batch 5 (2026-09-11):** `RageDisplay_D3D.cpp` (14 real sites,
  `Log::General` — no D3D-specific category exists, same reasoning as
  `CryptManager` in batch 2; using `Log::Gl` would have been
  misleading for a different rendering backend. 2 grep hits are
  pre-existing commented-out calls, left untouched). Triage: two real
  init-time failures upgraded to `LOG_ERROR` — `Direct3DCreate9`
  failing outright, and failing to find *any* usable back buffer
  format (`FindBackBufferType`'s terminal case, previously mislabeled
  `Trace`); the adapter display-mode query failure also upgraded
  `Warn`→`ERROR` (has a graceful fallback, but the query itself
  shouldn't normally fail). One clear mislabeling fixed:
  `TryVideoMode`'s entry trace was tagged `Warn` for no evident reason
  — it's a plain function-entry diagnostic, identical in kind to its
  (already-commented-out) sibling one line away in
  `RageDisplay_OGL.cpp` — downgraded to `LOG_TRACE`. For cross-backend
  parity with batch 4's already-`LOG_INFO` OGL vendor/mode startup
  dump, the equivalent D3D driver-identification and
  supported-display-mode dump (3 sites) was promoted `Trace`→`INFO`
  rather than left at the file's pre-existing level — same kind of
  information, same subsystem role, should behave the same under
  `--LogLevel` regardless of which renderer backend is active. The
  rest (routine per-mode/parameter-testing loop diagnostics) stayed at
  `LOG_TRACE`. No parsing/behavior logic changed; not §5-protected.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  Remaining: the long tail of non-§5 files (`StepMania.cpp`/
  `Profile.cpp`/etc. next), then the §5-protected
  parsers (`NotesLoaderSM.cpp` 39 sites, `CourseLoaderCRS.cpp` 27,
  `NotesLoaderSSC.cpp` 26, `Song.cpp` 25, etc.) last, as pure
  category/level re-tagging re-verified against their characterization
  tests — never a parsing-logic change.
  **Ph4 batch 6 (2026-09-11):** `StepMania.cpp` (23 real sites,
  `Log::General` — this is the top-level application/main-loop file,
  no single subsystem category fits; 3 grep hits are pre-existing
  commented-out input-debug calls, left untouched). Triage: kept the
  two existing `Warn` sites at `WARN` (a saved game-type preference
  that's no longer available, falling back to the default; an unknown
  `--game` command-line argument being ignored — both real, if
  recoverable, config/input problems, not routine). Downgraded one
  `Warn`→`INFO`: "video renderer list has been changed from X to Y" is
  the code noting a config divergence from card defaults and
  continuing normally — not a problem, just an FYI, so `WARN` was the
  wrong altitude for it. Everything else (startup banner/version/
  command-line-args dump, video-card-default detection, coin-mech
  bookkeeping, screenshot timing) was already correctly leveled
  `Trace`/`Info` and just got a category. No parsing/behavior logic
  changed; not §5-protected.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  **Ph4 batch 7 (2026-09-11):** `Profile.cpp` (16 sites, `Log::Profile`
  — a perfect category fit). Triage: the `LOAD_NODE(X)` macro's
  "Failed to read section X" upgraded `Warn`→`ERROR` — its 6 call
  sites (`GeneralData`, `SongScores`, `CourseScores`, `CategoryScores`,
  `ScreenshotData`, `CalorieData`) are all core, always-expected
  top-level sections of `stats.xml`; a missing one means real data
  loss/corruption, not an optional field. `LoadStatsFromDir`'s two
  file-open failures (plain open, and gunzip of the compressed
  variant) upgraded `Trace`→`ERROR` — both immediately return
  `ProfileLoadResult_FailedTampered` to the caller, i.e. the code
  itself already treats them as hard failures, the log level just
  hadn't caught up. `LoadSongsFromDir`'s "Song %s failed to load"
  (a custom song folder in the profile didn't load) upgraded
  `Trace`→`WARN` — a real, if per-item recoverable, failure worth
  surfacing to whoever's debugging why their custom song didn't show
  up. Everything else (routine load/save progress markers, signature-
  verification step-by-step tracing — actual signature *failures* are
  reported via `LuaHelpers::ReportScriptErrorFmt`, a separate path,
  not `LOG->`) stayed `Trace`. No parsing/behavior logic changed; not
  §5-protected.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  **This closes out the non-§5 tail of phase 4.** Everything left with
  a bare `LOG->` call is a §5-protected parser (`NotesLoaderSM.cpp` 39
  sites, `CourseLoaderCRS.cpp` 27, `NotesLoaderSSC.cpp` 26, `Song.cpp`
  25, `NotesLoaderBMS.cpp` 18, `NotesLoaderDWI.cpp` 14,
  `TimingSegments.cpp` 12, `NotesLoaderSMA.cpp` 12, `TimingData.cpp`
  11, `NotesLoaderKSF.cpp` 9, etc.) or the god-object `Player.cpp`
  (48) — both need a slower, more careful pass (characterization-test
  re-verification for the former; extra care for the latter given its
  size/hotspot status), not a routine batch.
  **Ph4 batch 8 (2026-09-12), first §5-protected batch:**
  `TimingSegments.cpp` (12 sites, `Log::Song`) and the 3 real
  (non-`UserLog`) sites in `NotesLoaderKSF.cpp` (also `Log::Song`; its
  6 `LOG->UserLog(...)` calls stay untouched, same out-of-scope
  reasoning as every other loader file — a separate user.txt facility
  with no `LOG_*` equivalent). `TimingSegments.cpp` turned out to be
  the cleanest possible pilot for the §5 lane: all 12 sites are
  identical-shape `DebugPrint()` overrides on each `TimingSegment`
  subclass (`BPMSegment`, `StopSegment`, `WarpSegment`, etc.), pure
  diagnostic formatting with zero decision logic — no triage judgment
  needed, all stayed `LOG_TRACE`. `NotesLoaderKSF.cpp`'s 3 sites
  (a `BPM`-conversion helper's trace, `LoadFromKSFFile`'s entry trace,
  `LoadFromDir`'s entry trace) were likewise already-correct routine
  `Trace` calls, just categorized. Re-verified against
  characterization tests before AND after the edit: `[TimingData]`
  40/9 and `[ksf]` 31/2, both identical — confirming pure category/
  level tagging with zero parsing/behavior change, per the §5 gate.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  Remaining §5 files (by size): `NotesLoaderSM.cpp` (39 raw hits, but
  most are likely `UserLog` given the pattern seen in every other
  loader — real count TBD per file), `CourseLoaderCRS.cpp` (27),
  `NotesLoaderSSC.cpp` (26), `Song.cpp` (25), `NotesLoaderBMS.cpp`
  (18), `NotesLoaderDWI.cpp` (14), `NotesLoaderSMA.cpp` (12),
  `TimingData.cpp` (11) — same treatment each time: filter out
  `UserLog`, triage only the real `Trace`/`Warn`/`Info` sites, re-run
  that file's characterization test before and after.
  **Ph4 batch 9 (2026-09-12):** `TimingData.cpp` (10 real sites,
  `Log::Song`), `NotesLoaderSMA.cpp` (1 real site), `NotesLoaderDWI.cpp`
  (2 real sites; a 3rd grep hit is a pre-existing commented-out call,
  left untouched), `NotesLoaderBMS.cpp` (2 real sites) — all
  `Log::Song`. Almost everything was already-correct routine `Trace`
  (lookup-table dumps, `AddSegment`/`EraseSegment` diagnostics, loader
  entry traces), just categorized. One real upgrade:
  `NotesLoaderDWI.cpp`'s "Didn't get enough data when attempting to
  load a DWI file" `Warn`→`ERROR` — a genuine malformed-chart parse
  failure that falls back to an empty `NoteData`, not routine.
  **Extra wrinkle handled:** 3 of `TimingData.cpp`'s sites
  (`EraseSegment`, `AddSegment`, the same-segment dedup check) sit
  behind `#ifdef WITH_LOGGING_TIMING_DATA` — a real, flippable CMake
  option (`option(WITH_LOGGING_TIMING_DATA ... OFF)`, unlike item 29's
  truly-orphaned networking code) that's off by default and so isn't
  exercised by the normal build gate. Reconfigured
  `-DWITH_LOGGING_TIMING_DATA=ON`, rebuilt, and reran the full suite
  (5966/226 unchanged) to actually compile-verify those 3 sites, then
  reconfigured back to the default `OFF` to match CI before finishing
  the gate. One more site sits behind `#ifdef DEBUG`, already covered
  by the normal Debug build. Re-verified against characterization
  tests: `[TimingData]` 40/9 (unchanged from batch 8's baseline);
  `[sma]` 74/2, `[dwi]` 30/2, `[bms]` 26/2 confirmed via the unchanged
  full-suite total (5966/226 both before and after this edit is the
  authoritative check, since any behavior change would have shifted
  it — no per-tag pre-edit baseline was captured for these three new
  tags specifically).
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  Remaining §5 files: `NotesLoaderSM.cpp`, `CourseLoaderCRS.cpp`,
  `NotesLoaderSSC.cpp`, `Song.cpp` — bigger, real-vs-`UserLog` split
  not yet broken down.
  **Ph4 batch 10 (2026-09-12):** `Song.cpp` (12 real sites, `Log::Song`
  — 2 grep hits are pre-existing dead code: one commented-out single
  line plus one whole `LOG->Trace(...)` call sitting inside a
  `/* ... */` block, both left untouched). Triage: the cache-load
  fallback warning ("main title or music file came up blank") kept at
  `WARN` (a real, self-healing data-quality issue with an actionable
  hint in the message itself). Three custom-song (profile-song)
  rejection sites upgraded `Trace`→`WARN` (too long / can't open music
  / file too big — each aborts loading that one custom song, matching
  the "Song %s failed to load" precedent from batch 7's
  `Profile.cpp`). "Points to a music file that doesn't exist, found
  music file X" upgraded `Trace`→`WARN` — a real simfile-authoring
  error (broken music-file reference) that the code recovers from via
  fallback, still worth surfacing. In `SaveToSSCFile`'s optional
  timestamped-backup step, the success case stayed `Trace` but the
  failure case upgraded `Trace`→`WARN` — the primary save already
  succeeded by this point, so it's not data loss, but a failed safety
  net is worth knowing about. The rest (save-flow entry traces for
  `.sm`/`.ssc`/`.json`/`.dwi`) stayed `Trace`. No parsing/behavior
  logic changed; verified via the unchanged full-suite total
  (5966/226) — no dedicated `[Song]`-tagged characterization test
  exists, so the full-suite invariant is the check here, same
  reasoning as batch 9's `[sma]`/`[dwi]`/`[bms]`.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  Remaining §5 files: `NotesLoaderSM.cpp`, `CourseLoaderCRS.cpp`,
  `NotesLoaderSSC.cpp` — the three biggest, real-vs-`UserLog` split
  not yet broken down.
  **Ph4 batch 11 (2026-09-12) — closes out the entire §5-protected
  lane.** `NotesLoaderSM.cpp` (1 real site — its other 3 raw grep hits
  were all pre-existing commented-out calls, not real sites despite
  the "39" raw count originally estimated), `CourseLoaderCRS.cpp`
  (3 real sites), `NotesLoaderSSC.cpp` (2 real sites; 1 more grep hit
  is a pre-existing commented-out call) — all `Log::Song` (no
  dedicated `Course` category exists either, same catch-all
  reasoning). Every single site across all three files turned out to
  already be a correctly-leveled routine `Trace` (loader/edit-file
  entry points, cache-vs-fresh-load branch tracing) — no triage
  upgrades needed, purely categorization. Verified against real
  characterization coverage: `[SMLoader]` 43/14, `[corpus]` 313/3
  (covers both `.sm` and `.ssc` via the paired-format test), `[crs]`
  39/5 — all unchanged, plus the full suite (5966/226).
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  **Item 18 phase 4 is now fully done except `Player.cpp`** (48 sites,
  god-object hotspot — its own careful pass, no characterization test
  exists for it since it's gameplay/scoring logic, not simfile
  parsing). Every §5-protected simfile-format file has been migrated
  to the categorized `LOG_*` macros with zero parsing/behavior changes
  throughout, each re-verified against its characterization test (or
  the full-suite invariant where no per-format tag existed).
  **Ph4 final batch (2026-09-12), `Player.cpp` — phase 4 now
  COMPLETE.** Of the 48 raw `LOG->` grep hits in this god-object
  (3567 lines, a hotspot per item 9), a programmatic check (`awk`
  tracking `/* */` block-comment state, not just skipping `//` lines)
  found that **47 of the 48 are dead code** — either single-line `//`
  comments or sitting inside multi-line `/* ... */` blocks — leftover
  debug scaffolding from a historically fragile hold-note-scoring
  area (the same section item 15 already flagged as "too fragile a
  hot path to guess at" for `#if 0` removal). Only **one** call site
  is actually compiled: a routine "Applying transform '%s'..." trace
  in the attack-mod application path, migrated to
  `LOG_TRACE(Log::Actor, ...)` (`Actor` since `Player` is a gameplay
  `Actor` subclass; no dedicated category fits gameplay/scoring logic
  any better). No triage judgment needed — already correctly `Trace`.
  No characterization test exists for `Player.cpp` (gameplay/scoring,
  not simfile parsing), so the full-suite invariant (5966/226
  unchanged) is the only available safety check, same as `Song.cpp`
  in batch 10.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  **CORRECTION (2026-09-12, same day): phase 4 was declared complete
  prematurely.** The batches above were all worked from a **fixed
  top-30 list captured once at the very start of phase 4 recon**
  (~799 raw hits counted then); every batch since picked files off
  that original list, but the list itself was never a full sweep, and
  no final re-sweep of the whole `src/` tree was done before declaring
  victory. A fresh sweep (`grep -cE "LOG->Trace|LOG->Warn|LOG->Info"
  src/*.cpp`, deliberately excluding `LOG->UserLog` which stays
  out-of-scope) after the `Player.cpp` batch found **127 more files**
  with real, un-migrated sites (~430+ sites total) that were simply
  never on the original top-30 — `PlayerStageStats.cpp` (12),
  `ScreenGameplay.cpp` (11), `RageUtil.cpp` (11), `ScreenManager.cpp`
  (10), `RageDisplay_GLES2.cpp` (10), and a long tail down to
  1-site files. **This is the same methodology trap already
  documented for item 2's C4244/C4267 sweep** ("declared complete
  wrongly at least four times before the true zero") — a curated
  work-list from one point in time is not a substitute for a final
  full-tree re-sweep. Phase 4 continues from here as a real "long
  tail, per subsystem" the way ADR 0005 always described it, no
  longer batching off the stale original list — see the batches
  below for the corrected, ongoing count.
  **Ph4 batch 12 (2026-09-12), first batch against the corrected
  remaining list.** `PlayerStageStats.cpp` (1 real site — the other 11
  raw hits were pre-existing commented-out calls, `Log::Lua` — a bad
  argument from a theme's `GetLifeRecord` Lua call, upgraded
  `Trace`→`WARN`), `RageUtil.cpp` (11 sites, split by function:
  `GetFileContents`/`FileCopy` failures → `Log::File` + `ERROR` for
  genuine I/O failures, kept `WARN` for the "copy over itself" misuse
  guard; `StringToInt/Long/LLong`'s catch-block warnings →
  `Log::General`, kept `WARN` since this utility is also used for
  legitimate speculative "maybe it's a number" parsing, not just hard
  errors), `ScreenGameplay.cpp` (9 real sites, `Log::Screen` — "Error
  loading notes for player" upgraded `Trace`→`ERROR`, a genuine
  simfile-load failure; everything else already-correct routine
  gameplay-event `Trace`), `ScreenManager.cpp` (8 real sites,
  `Log::Screen`, all already-correct routine screen-lifecycle
  `Trace`), `RageDisplay_GLES2.cpp` (10 sites, `Log::Gl` — same
  vendor/extension-dump→`INFO` and mislabeled-`TryVideoMode`-entry-
  trace→`TRACE` pattern already established for `RageDisplay_OGL.cpp`/
  `RageDisplay_D3D.cpp`; **this file is Linux-only**,
  `elseif(LINUX) if(WITH_GLES2)`-gated in `CMakeData-rage.cmake`, so
  the Windows build never compiles it — `WITH_GLES2` defaults `ON` and
  Ubuntu CI does build it, so that CI leg going green is this file's
  real compile verification, not the local Windows gate),
  `ScreenSelectMusic.cpp` (8 real sites, `Log::Screen` — the two
  song-deletion guard warnings kept `WARN`, everything else routine
  `Trace`). No parsing/behavior logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0 (Windows); the
  `RageDisplay_GLES2.cpp` site still needs Ubuntu CI to confirm.
  ~121 files remain on the corrected list (`ScreenEdit.cpp` (7),
  `RageUtil_BackgroundLoader.cpp` (7), `MusicWheel.cpp` (7),
  `ImageCache.cpp` (7), `Bookkeeper.cpp` (7), `Font.cpp` (8),
  `RageDisplay.cpp` (8), and a long tail down to 1-site files).

### 20. Replace the archaic hard-coded game-type system
Game types are defined by hand-written `static const Game g_Game_X = {…}`
struct literals in `src/GameManager.cpp` (~150 lines each: controllers,
button maps, per-style mappings, menu buttons), registered in a
hand-maintained `g_Games[]` array, with a **compile-time `StepsType`
enum** and a parallel `g_StepsTypeInfos[]` array. Adding or editing a game
means editing a 3600-line `.cpp` and rebuilding. All game types are now
enabled (`229d0769d5`); the *mechanism* is what needs replacing.
**Goal:** a **data-driven game/style/stepstype registry** — defined in
data (Lua or a `Games/` tree, like NoteSkins/Themes are), loaded at
startup, with the C++ side working off a runtime id instead of the
`StepsType` enum.

**End-state — `NoteSkins/` is the switch for supported games.** A game is
offered iff `NoteSkins/<gamename>/` holds ≥1 valid skin. This is *already*
how enablement works (`GameManager::GetEnabledGames` →
`IsGameEnabled` → `NoteSkinManager::DoNoteSkinsExistForGame`); after
`229d0769d5` `g_Games[]` lists all defined games, so the NoteSkins check
is the only gate that reaches the UI. What remains: the `g_Games[]` array
+ `g_Game_*` structs are still hand-maintained C++. Target: adding a game
= drop in a definition file + a `NoteSkins/<name>/` folder — no `.cpp`
edit, no rebuild. `g_Games[]` becomes "every definition found", the
NoteSkin dir stays the on/off.
**Hard constraint:** the on-disk `#STEPSTYPE` strings (`dance-single`,
`pump-double`, `bm-single7`, `pnm-nine`, …) are a stable contract with the
~20-year simfile library (`AGENTS.md` §5) — every existing value must
resolve identically, no mass cache invalidation.
**Scope:** big. `StepsType` (enum → runtime id) ripples through
`NoteData`, `Style`, `Steps`, `RadarValues`, score keepers, the editor,
Lua bindings, `Profile` serialization. **Deserves its own ADR** when
picked up. Also: `NoteSkins/Para/` is capitalised but the game name is
`para` — rename for Linux (case-sensitive FS).

### 29. Networking/multiplayer (SMOnline) subsystem is orphaned from the CMake build
Found 2026-09-11 while doing item 18 phase-4 batch 3: `NetworkSyncManager.cpp`
(and, going by the same grep, its whole calling subsystem —
`ScreenSMOnlineLogin.cpp`, `ScreenNetSelectMusic.cpp`,
`ScreenNetworkOptions.cpp`, `ScreenNetRoom.cpp`, `ScreenNetSelectBase.cpp`,
`ScreenNetEvaluation.h`, `RoomWheel.cpp`, `RoomInfoDisplay.cpp`) appears in
**none** of the `src/CMakeData-*.cmake` source lists and produces no
`.obj` in a from-scratch build — confirmed by grepping every
`CMakeData-*.cmake` for these filenames (zero hits) and checking the
`sm_engine.dir` build tree for their object files (none exist). The
`.cpp`/`.h` files are still present in the tree and still reference
each other (`NetworkSyncManager.h` is `#include`d by the Screen* files),
but none of it is reachable from a normal build today — this predates
this modernization effort and most likely dates to when the old
autotools/scons build systems were dropped (item 22) and the CMake
source lists were hand-curated without carrying this subsystem over.
**Not investigated further yet** (maintainer chose to note-and-defer
rather than open a side investigation into how broken/complete it is).
Two ways this could go, both needing a maintainer call: (a) wire it
back into `CMakeData-*.cmake` and get it building again (unknown effort
— it hasn't compiled against the current codebase in an unknown
amount of time, so there may be real rot under the surface), or
(b) treat it as confirmed-dead and remove it outright (same category as
item 19's `CreateZip`/item 23's `smpackage`). Until decided, the item
18 phase-4 logging migration still applied to `NetworkSyncManager.cpp`
(pure category/level tagging, zero risk either way since the file isn't
compiled) — see item 18's batch 3 note.

---

## Closed

- **Item 3** — `Docs/Devdocs/possible memory leaks.txt`, a 2009 cppcheck
  leak list never re-verified (2026-09-05). Re-triaged all 11 in-scope
  entries by hand (cppcheck not installed locally; read each ownership
  path directly). Result: zero live leaks. Most are false positives
  cppcheck's simple checker can't model — manager-owned resources
  (`ActorFrameTexture`), sound-reader chains (`AutoKeysounds`,
  `RageSoundReader_PitchChange`), explicit refcounting
  (`RageSoundReader_ChannelSplit`), the Actor-tree `AddChild`/
  `DeleteAllChildren` idiom (`OptionRow`), Lua-script-managed lifetime
  (`RageFile`), or a deliberate Meyer's-singleton (`RageThreads`'s
  `GetThreadSlotsLock()`). A few were already fixed since 2009
  (`Font.cpp`'s `pPage`; `RageFileDriverDeflate.cpp`'s `mem`, now
  `std::unique_ptr`-owned). `AdjustSync::s_pTimingDataOriginal` no
  longer exists (refactored to a `std::vector` value member).
  `PitchDetectionTestUtil.cpp` and `crypto/CryptRSA.cpp` are gone.
  Two non-Windows entries left unevaluated per `AGENTS.md` §3. Docs-only
  change — no code touched, no rebuild needed. Full reasoning recorded
  in the leak-list file itself.
- **Item 14** — `vcvars64.bat` doesn't wire the Windows SDK
  (2026-09-04). Shipped `Build/dev-env.ps1` (the "ship a project `env`
  helper script" option) instead of repairing the VS install: runs
  `vcvars64.bat` (located via `vswhere.exe`), and if `WindowsSdkDir`
  still comes back empty, wires the newest installed SDK version
  (auto-detected, not hardcoded) into `INCLUDE`/`LIB`/`PATH`. Verified
  end-to-end on the maintainer box: compiled + ran a `<windows.h>`
  program linked against `kernel32.lib`, and `cmake -G Ninja -B build`
  configures clean (previously failed compiler/SDK detection without
  it — the actual documented pain point).
- **Item 19** — OS version detection reported "Windows 8" on Windows 11
  (2026-09-04, `fee51d41e7`). Root cause: the exe manifest had no
  `<compatibility>` `supportedOS` entries, so Windows caps
  `GetVersionEx`/`RtlGetVersion` at 6.2 for any unmanifested app;
  added the Windows 10 GUID (covers 11 too — no separate GUID exists,
  identified only by build ≥ 22000). `DebugInfoHunt.cpp`'s version
  table also had no branch for major version 10 at all. Verified via a
  real `--SelfTest` run: `Logs/info.txt` now reads
  `Windows 10.0 (Win11) build 26200`.
- **Item 8** — unsafe C string ops in the Windows crash/URL/zip paths
  (2026-09-04), scoped to what was in-scope (P1 platform; `archutils/
  Unix/CrashHandler*` untouched per `AGENTS.md` §3). Three commits:
  `GotoURL.cpp` (`d346dacccc`) — the one genuine, reachable overflow:
  a fixed `char[2*MAX_PATH]` `strcat`'d with `sUrl`, which reaches here
  network-supplied via the crash handler's update checker
  (`CrashHandlerChild.cpp`'s `m_sUpdateURL`, parsed from the
  update-check XML response). Confirmed `GotoURL` only ever runs in
  the crash handler's separate child process (`CreateProcess`-spawned,
  not the crashed process's own exception handler) or normal app code,
  so rewrote with `RString` (no fixed capacity to overflow) — safe
  because heap allocation is fine there, unlike `Crash.cpp` itself.
  `Crash.cpp` (`0095f2673d`) — `SpliceProgramPath`, `StartChild`,
  `CrashGetModuleBaseName`: none reachable with an attacker-controlled
  length today, but this file explicitly forbids `malloc`/`new`
  (crash-time), so fixed with bounded `strncpy`-style copies + explicit
  length math instead of a growable string type.
  `CreateZip.cpp` (`448e4412fe`) — `TZip::Add`'s entry-name `_tcscpy`
  into a fixed buffer, rejected instead of overflowed if too long;
  `TZip`/`CreateZip` have no callers anywhere in the current `src/`
  tree, hardened for whenever it's wired up. All three verified against
  a Windows Release build clean under `WITH_WERROR=ON` (`/WX`) +
  `--SelfTest`.
- **Item 7** — `extern/ffmpeg-w32/` (36 MB committed blob, unknown
  provenance) replaced with a CI-built artifact from the pinned
  `extern/ffmpeg` submodule (2026-09-04). `.github/workflows/
  build-ffmpeg-win32.yml` cross-compiles with mingw-w64 on
  `ubuntu-latest` (no Windows runner needed) and generates MSVC `.lib`
  import libs via `gendef` + `llvm-lib`; published as GitHub Release
  `ffmpeg-w32-19feb712f5`. `CMake/SetupFfmpegWin32.cmake` downloads +
  SHA256-verifies it into `extern/ffmpeg-w32-prebuilt/` at configure
  time (offline on repeat configures). `StepmaniaCore.cmake`,
  `src/CMakeLists.txt`, `tests/CMakeLists.txt` point at that dir now.
  Verified locally (fresh network download, Release build + `--SelfTest`,
  `WITH_TESTS` Debug build) and in CI (all 8 jobs green, including the
  Windows runner downloading the same Release asset). Deviates from the
  original recipe by dropping `--enable-bzlib`/`-zlib` (no Ubuntu
  mingw-w64 package for either; unused by StepMania's codec paths).
- **Item 4** — dead Travis + AppVeyor CI configs removed (`e065f69c8b`, 2026-09-03).
- **Item 5** — orphaned `src/irc/` IRC-reporter subproject removed (`718d3b3ec1`, 2026-09-03).
- **Item 6** — `Build/README.md` + `Build/INSTALL.md` rewritten to current
  reality (CMake 3.20, `cmake -B build`, Windows 11 floor); `build.md`
  cross-checked (2026-09-03).
- **Item 13 (mostly)** — `arch_setup.h`: dead `isnan`/`isfinite` macros
  (`37e6766d5e`); `_WIN32_WINNT`/`_WIN32_IE` → `0x0A00`, `__STDC__ 0`
  removed (`5565039bf7`). Only warning-suppression cruft left.
