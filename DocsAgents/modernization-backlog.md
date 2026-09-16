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
- **`src/archutils/Win32/ddk/`** — **DELETED 2026-09-14** (`641d903cff`).
  `USB.cpp` (live code — the Para/Pump dance-pad HID drivers, not
  orphaned) was the only file including anything from this directory;
  migrated its two `#include "archutils/Win32/ddk/..."` lines to the
  Windows 10 SDK's own `<setupapi.h>`/`<hidsdi.h>` (already on the
  include path via `<windows.h>`). Diffed the vendored vs SDK headers
  for every struct/function `USB.cpp` actually uses
  (`HIDD_ATTRIBUTES`, `HidD_GetHidGuid`, `HidD_GetAttributes`,
  `SP_DEVICE_INTERFACE_DATA`, `SP_INTERFACE_DEVICE_DETAIL_DATA`,
  `SetupDiGetClassDevs`/`EnumDeviceInterfaces`/
  `GetDeviceInterfaceDetail`/`DestroyDeviceInfoList`) — identical
  layouts, the SDK just adds modern SAL annotations; the legacy
  `SP_INTERFACE_DEVICE_DETAIL_DATA` alias this code relies on still
  exists in the current SDK for back-compat. The 6 `.lib` files went
  with the headers (CMake already links these 3 libs from the SDK path
  directly, `src/CMakeLists.txt:400-402` — nothing else changed).
  Verified via a genuine clean rebuild (CMake reconfigure + forced
  recompile) after physically deleting the files from disk, not just
  an incremental build that could mask a missing include. 12,522 lines
  / ~480 KB removed.
- **`Xcode/Libraries/*.a`** — **DELETED 2026-09-14** (`f82c2428f0`),
  along with the rest of `Xcode/`'s dead pre-CMake artifacts found
  while investigating this item. Kept only the 2 files the current
  CMake macOS build actually uses (`Info.plist.in`, `smicon.icns`,
  referenced via `SM_XCODE_DIR` in `src/CMakeLists.txt:258,294`) plus
  `README.md` (already correctly documents that the old Xcode-project
  workflow is gone). Removed: `Libraries/*.a` (2010-era prebuilt jpeg/
  ogg/png/vorbis/theora/zlib/ffmpeg — every one superseded by a modern
  `extern/`-built equivalent; theora isn't used in `src/` at all
  anymore), `Patcher/` (a standalone dead Obj-C updater GUI with its
  own `.xcodeproj`), `scripts/` (`mkrelease.rb`'s own README says it
  "assumes you have already built a StepMania.app using Xcode" — the
  workflow `Xcode/README.md` itself says no longer exists;
  `increment_version.pl` is superseded by `src/verstub.in.cpp` +
  CMake, item 25), the 6 `.lproj/Localizable.strings` dirs, and 6 more
  unreferenced loose files (`Hardware.plist`/`.in.plist`,
  `Info.plist.in.xml`, `plistHelper.in.hpp`, `product.xcconfig`,
  `StepMania.entitlements`). Zero references anywhere in CMake/CI/docs
  for any of it, confirmed via grep before deletion. 46 files, ~8.3 MB.
  Verified with a real CMake reconfigure + Release/Debug rebuild
  locally (Windows-only box, so the macOS `.app` bundle step itself
  needed CI's `macOS (arm64)` job as the real verification, not just
  local green). **This fully closes item 26 — nothing left in it.**

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
**Phase 1, cluster 1 (2026-09-13): `GameState`'s "Edit stuff" carved
out into `GameStateEditData.h`/`.cpp`.** Maintainer asked for this
cluster specifically. Moved 8 members (`m_bIsUsingStepTiming`,
`m_bInStepEditor`, `m_stEdit`, `m_cdEdit`, `m_pEditSourceSteps`,
`m_stEditSource`, `m_iEditCourseEntryIndex`, `m_sEditLocalProfileID`)
and `GetEditLocalProfile()`'s body into a new owned `GameStateEditData`
member (`m_EditData`). Registered in `CMakeData-singletons.cmake` next
to `GameState.cpp`/`.h`.
**Genuinely achieved zero call-site churn** (the playbook's phase-1
goal) for *public data members*, not just accessor methods — these
were raw public fields (`GAMESTATE->m_stEdit` etc.), touched directly
across 13 files (~112 sites: `ArrowEffects.cpp`, `NoteDisplay.cpp`,
`NoteField.cpp`, `OptionRowHandler.cpp`, `Player.cpp`,
`PlayerState.cpp`, `ScreenEdit.cpp`, `ScreenOptionsEditProfile.cpp`,
`ScreenOptionsManageCourses.cpp`, `ScreenOptionsManageProfiles.cpp`,
`Steps.cpp`, plus `GameState.cpp` itself including its `Luna<GameState>`
Lua binding block) — none of them needed touching. `GameState.h` now
declares these as **reference members** (`bool& m_bIsUsingStepTiming;`
etc.) bound in the constructor's init-list to the corresponding
`m_EditData.xxx` field; every existing read/write/`.Set()` call
continues to compile and behave identically, since a reference member
transparently forwards to its referent. This is safe specifically
because `m_EditData` is a same-lifetime value member of `GameState`
(never copied — `GameState`'s copy ctor/assignment are already
`private`/undefined) and is declared before the reference members in
the class body, so it's fully constructed before they bind to it.
`GetEditLocalProfile()` stays a thin inline forwarding method on
`GameState` per the playbook's step 4 (`{ return
m_EditData.GetEditLocalProfile(); }`). Reset parity (playbook gotcha)
is automatic: `GameState::Reset()`'s existing `m_stEdit.Set(...)` etc.
lines were not touched at all, they still write through the reference
to the same underlying storage.
Verified: `sm_tests` 5966/226 unchanged (at the point of this specific
change, before the separate CoinMode test below), `ctest` 100%,
Release `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0. **Phase 2
(migrating call sites to talk to `m_EditData` directly) is
deliberately not attempted** — per the playbook, that's a separate,
later PR if ever wanted.

**Phase 1, cluster 2 (2026-09-13): `GameState`'s "used in workout"
fields carved out into `GameStateWorkoutData.h`/`.cpp`.** Even smaller
blast radius than cluster 1: only 3 files touch this cluster at all
(`GameState.h`/`.cpp` and `ScreenGameplay.cpp`, 1 read site). Moved
`m_bGoalComplete[NUM_PLAYERS]`, `m_bWorkoutGoalComplete`, and
`GetGoalPercentComplete()`'s full body (`IsGoalComplete()` is a
1-liner that already just called it) into the new component —
`GetGoalPercentComplete()` only ever touched `PROFILEMAN`/`STATSMAN`
(external singletons) and its `pn` parameter, no other `GameState`
state, so it moved as a real implementation, not just a forward.
Same reference-member technique as cluster 1, with one new wrinkle:
**`m_bGoalComplete` is an array (`bool[NUM_PLAYERS]`), not a scalar** —
a reference to an array member uses the `T (&name)[N]` declarator
(`bool (&m_bGoalComplete)[NUM_PLAYERS];`), which binds and indexes
exactly like the original array (`m_bGoalComplete[p] = false;` in
`GameState::Update()`/`ResetStageStatistics()` needed zero changes).
`GetGoalPercentComplete()`/`IsGoalComplete()` stay thin inline
forwards on `GameState` per playbook step 4. No new characterization
test added — like cluster 1, this is a pure mechanical relocation with
identical behavior, covered by the existing full suite + build gate,
not new logic that needs pinning.
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Phase 1, cluster 3 (2026-09-13): `GameState`'s "Attract stuff"
carved out into `GameStateAttractData.h`/`.cpp`.** Picked after
scouting "Award stuff" (`m_vLastStageAwards`/`m_vLastPeakComboAwards` +
`StageAward`/`PeakComboAward` types) and finding it touches 8 files
including `HighScore.cpp`/`.h`, `PlayerStageStats.cpp`/`.h`,
`StageStats.cpp` — those types are used broadly across the scoring
pipeline, not just as passive `GameState` fields, so a much bigger
blast radius than expected; deferred. "Attract stuff" is the smallest,
cleanest fit: only 4 files total (`GameState.h`/`.cpp`,
`ScreenAttract.cpp`, `ScreenDemonstration.cpp` — the latter two only
ever call the two methods below, never touch the field directly).
Moved `m_iNumTimesThroughAttract` (a plain `int`, not wrapped in
`BroadcastOnChange`) and both of its methods' full bodies —
`IsTimeToPlayAttractSounds()`/`VisitAttractScreen()` only ever touched
`PREFSMAN`/`CommonMetrics` (external singletons) and the moved field,
no other `GameState` state, so they moved as real implementations.
Same reference-member technique (a plain `int&` this time, no new
wrinkle needed). `GameState`'s constructor still had a body-level
`m_iNumTimesThroughAttract = -1;` line predating this split (in
addition to `GameStateAttractData`'s own constructor now also
initializing it to -1) — left as harmless-but-redundant rather than
"improving" it beyond the split's minimal diff.
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Phase 1, cluster 4 (2026-09-13, autonomous — "completa el backlog
solo con lo no bloqueante" goal): `GameState`'s "Autogen stuff" carved
out into `GameStateAutogenData.h` (header-only, no `.cpp` — the whole
class is one inline getter + one vector, matching how it already lived
inline in `GameState.h`).** The smallest cluster yet: only
`GameState.h`/`.cpp` and one external read site
(`NoteDataUtil.cpp:837`, `GAMESTATE->GetAutoGenFarg(0)`) touch it — the
original author's own comment already flagged this as should-be-its-
own-thing ("This should probably be moved to its own singleton or
something when autogen is generalized and more customizable. -Kyz").
Moved `m_autogen_fargs` (`std::vector<float>`) and `GetAutoGenFarg()`'s
body. `GameState.cpp`'s `Luna<GameState>` Lua-binding block has two
thunks (`GetAutoGenFarg`/`SetAutoGenFarg`) that directly call
`p->m_autogen_fargs.push_back(...)`/`.size()`/`[si]=...` — all kept
compiling unchanged via the reference member, same as every other
cluster's internal `Luna` usage. Reference member bound via the
constructor's init-list as usual (not an in-class default member
initializer) for consistency with the other three clusters in this
same file, even though an NSDMI would have worked too and let the
`.cpp` constructor go untouched.
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Phase 1, cluster 5 (2026-09-13, autonomous — same standing goal):
`GameState`'s "Haste" fields carved out into `GameStateHasteData.h`
(header-only, like cluster 4 — 3 plain data members, nothing to put in
a `.cpp`).** Pure data, no associated methods at all — `m_fHasteRate`/`m_fLastHasteUpdateMusicSeconds`/
`m_fAccumulatedHasteSeconds` (3 plain floats) are read/written directly
as `GAMESTATE->m_fHasteRate` etc. from `ScreenGameplay.cpp` (13 sites,
all direct field access — checked each one before committing to this
cluster, per the "verify full footprint" lesson from rejecting Award
Stuff; none of them turned out to be more than simple reads/writes/
`CLAMP()`/`SCALE()` calls on the raw values) and reset in
`GameState::ResetStageStatistics()`. None of the three fields were
previously in `GameState`'s constructor init-list (left uninitialized
until `Reset()` runs, same as most plain-float `GameState` members) —
`GameStateHasteData` correctly has **no explicit constructor**, since
adding one that zero-initializes would silently change behavior versus
the original (implicitly uninitialized until first `Reset()`).
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0. **5 clusters done
now (Edit, Workout, Attract, Autogen, Haste).**

**Phase 1, cluster 6 (2026-09-13): `GameState`'s MultiPlayer-mode
fields carved out into `GameStateMultiPlayerData.h` (header-only).**
The first cluster that's genuinely **not a single contiguous block**
in the header — `m_MultiPlayerStatus`, `m_bMultiplayer`/
`m_iNumMultiplayerNoteFields`, and `m_pMultiPlayerState` sit at three
separate spots, interleaved with unrelated `GameState` members
(`m_PlayMode`, `m_iCoins`, a dozen `ChangePreferredDifficulty*`
methods, etc.) and with the core 2-player fields
(`m_bSideIsJoined`, `m_pPlayerState`) that were **deliberately left
alone** as too foundational. This didn't require moving the fields
together textually — each one became a reference member *in its
original location*, as long as `GameStateMultiPlayerData
m_MultiPlayerData;` itself was declared before all of them (placed
right where `m_MultiPlayerStatus` used to sit, the first of the four
in declaration order). Scouted the real blast radius first: ~34 sites
across 9 external files, but only `GameState.cpp` (18) and
`ScreenGameplay.cpp` (8) had more than 1-2 touches each — everything
else was a single incidental read.
**A new kind of member for this technique: `m_pMultiPlayerState` is a
heap-allocated pointer array** (`new PlayerState` per slot in
`GameState`'s constructor *body*, `SAFE_DELETE` in the destructor,
both loops already living outside the member-initializer list). Since
the reference member only needs *binding* (in the init-list) to the
component's real array storage, the existing allocation/deallocation
loops in the constructor/destructor body needed **zero changes** —
`new`/`delete`/array-indexing all pass through a reference-to-array
exactly like through the original array. `IsMultiPlayerEnabled()`
moved as a real implementation (only touched
`m_MultiPlayerStatus[mp]`, no other `GameState` state); the free
function `GetNextEnabledMultiPlayer()` calls `GAMESTATE->
IsMultiPlayerEnabled(mp)` and needed no changes at all, since it goes
through the still-identical public method. None of the four fields
were in the original constructor init-list (left uninitialized until
`Reset()`), so `GameStateMultiPlayerData` correctly has no explicit
constructor, matching the Haste-cluster precedent.
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0. **6 clusters done
now (Edit, Workout, Attract, Autogen, Haste, MultiPlayer).**

**Phase 1, cluster 7 (2026-09-13): `GameState`'s "used by themes that
support heart rate entry" fields carved out into
`GameStateDanceData.h` (header-only).** `m_DanceStartTime`
(`RageTimer`) and `m_DanceDuration` (`float`) — pure data, no
associated methods. Small external footprint (only `ScreenGameplay.cpp`
touches them, via `.Touch()`/`.Ago()` and a plain read/write). Neither
field was in the original constructor init-list — `m_DanceStartTime`
being a class type (`RageTimer`) still gets correctly default-
constructed via `GameStateDanceData`'s own implicit default
constructor (unlike POD fields, class-type members are always
default-constructed even with no explicit initializer, so this
requires no special handling — just confirm the *new* component's
constructor doesn't accidentally override that with an explicit one).
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0. **7 clusters done
now (Edit, Workout, Attract, Autogen, Haste, MultiPlayer, Dance).**

**Phase 1, cluster 8 (2026-09-13): `GameState`'s `PLAY_MODE_BATTLE`/
`PLAY_MODE_RAVE` fields carved out into `GameStateBattleRaveData.h`
(header-only).** `m_fOpponentHealthPercent` and `m_fTugLifePercentP1`
(both `float`) — pure data, no associated methods. Recursive grep
across all 6 touching files (`CombinedLifeMeterTug.cpp`,
`Inventory.cpp`, `Player.cpp`, `ScoreKeeperRave.cpp`,
`ScreenGameplay.cpp`, `GameState.cpp`) confirmed only plain
reads/writes/comparisons, no hard boundaries. Neither field was in
the original constructor init-list, so `GameStateBattleRaveData`
correctly has no explicit constructor — same shape as the
Haste/Dance clusters, no new technique wrinkle. Verified: `sm_tests`
5981/230 unchanged, `ctest` 100%, Release `StepMania-R.exe` clean
rebuild, `--SelfTest` exit 0. **8 clusters done now (Edit, Workout,
Attract, Autogen, Haste, MultiPlayer, Dance, BattleRave).**

**Phase 1, cluster 9 (2026-09-13): `GameState`'s per-game/round random
seed fields carved out into `GameStateStageSeedData.h` (header-only).**
`m_iGameSeed`, `m_iStageSeed` (both `int`), `m_sStageGUID` (`RString`,
left as `RString` — this is an item-9 split only, not an item-10
RString migration) plus `SetNewStageSeed()`, moved as a real
implementation since it only touches `m_iStageSeed` (`rand()` is a
free function). 5 external files (`NoteDataUtil.cpp`, `StatsManager.cpp`,
`Course.cpp`, `ScreenGameplay.cpp`, `ArrowEffects.cpp`) all touch these
only via plain reads/the public `SetNewStageSeed()`/`GAMESTATE->`
accessors — no hard boundaries. `NoteDataUtil.cpp`/`Course.cpp` are
§5-adjacent, so re-verified `[corpus]` (313/3) and `[crs]` (39/5)
unchanged before/after. Neither int field nor the RString was in the
original constructor init-list, so the new component correctly has no
explicit constructor. Verified: `sm_tests` 5981/230 unchanged, `ctest`
100%, Release `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
**9 clusters done now (Edit, Workout, Attract, Autogen, Haste,
MultiPlayer, Dance, BattleRave, StageSeed).**

**Phase 1, cluster 10 (2026-09-13): `GameState`'s "character stuff"
field carved out into `GameStateCharacterData.h` (header-only).**
`m_pCurCharacters[NUM_PLAYERS]` (`Character*`, array reference member
— same shape as cluster 6's `m_MultiPlayerStatus`). Recursive grep
across all 6 touching files (`AttackDisplay.cpp`, `BeginnerHelper.cpp`,
`DancingCharacters.cpp`, `GameCommand.cpp`, `ScoreKeeperRave.cpp`,
`ScreenSelectCharacter.cpp`) found only plain indexing/assignment/
comparison, no hard boundaries — none are §5-adjacent (UI/scoring, not
simfile parsing). Field was not in the original constructor init-list,
so `GameStateCharacterData` correctly has no explicit constructor.
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0. **10 clusters
done now (Edit, Workout, Attract, Autogen, Haste, MultiPlayer, Dance,
BattleRave, StageSeed, Character).**

**Phase 1, cluster 11 (2026-09-13): `GameState`'s private "Timing
position corrections" fields carved out into
`GameStatePositionCorrectionData.h` (header-only).**
`m_LastPositionTimer` (`RageTimer`), `m_LastPositionSeconds` (`float`),
`m_paused` (`bool`) — already `private` to `GameState`, so zero
external exposure to begin with; only touched inside
`GameState::ResetMusicStatistics()`/`UpdateSongPosition()` plus the
trivial inline `SetPaused()`/`GetPaused()`. No methods moved (both
touching methods also use unrelated `GameState` state, so they stay on
`GameState` and just read through the reference members
transparently). None of the three fields were in the original
constructor init-list, so the new component correctly has no explicit
constructor. Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%,
Release `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0. **11
clusters done now (Edit, Workout, Attract, Autogen, Haste,
MultiPlayer, Dance, BattleRave, StageSeed, Character,
PositionCorrection).**

**Phase 1, cluster 12 (2026-09-13): `GameState`'s MusicWheel expanded/
last-open section fields carved out into `GameStateSectionData.h`
(header-only).** `sExpandedSectionName` and `sLastOpenSection` (both
`RString`, contiguous in the header, thematically related — both
track which song-group section is expanded/was last open on the
MusicWheel). Only 2 external files touch the `GAMESTATE->` fields
directly (`MusicWheelItem.cpp`, `MusicWheel.cpp`; `WheelBase.cpp` and
`MusicWheel.cpp`'s own `m_sExpandedSectionName` are a *different*,
same-named local member on those wheel classes, not this field) — all
via plain `==` comparisons or assignments, no hard boundaries. Neither
field was in the original constructor init-list, so the new component
correctly has no explicit constructor. Verified: `sm_tests` 5981/230
unchanged, `ctest` 100%, Release `StepMania-R.exe` clean rebuild,
`--SelfTest` exit 0. **12 clusters done now (Edit, Workout, Attract,
Autogen, Haste, MultiPlayer, Dance, BattleRave, StageSeed, Character,
PositionCorrection, Section).** Remaining `GameState.h` fields are
either already-rejected coupled clusters (Ranking Stuff, Award stuff,
stage-token cluster) or isolated single bools
(`m_bDopefish`, `m_bLoadingNextSong`, `m_bBackedOutOfFinalStage`,
`m_bTemporaryEventMode`) with no natural thematic grouping — forcing
them into one artificial component would be exactly the kind of
unneeded abstraction the project avoids, so item 9's phase-1 low-risk
clusters are considered exhausted for now without a maintainer call on
how (or whether) to group the leftovers.

**Related (2026-09-13): "why does Pay mode do nothing?" investigated
and answered — nothing was disabled.** Maintainer recalled StepMania
used to have Home/Free/Pay coin modes and asked to "reactivate" Pay.
Investigation (git history + live code read) found `CoinMode_Pay` is
**fully implemented and wired** today — credits/coins math in
`GameCommand.cpp`/`GameState.cpp`, the 3-way "Home"/"Pay"/"Free Play"
`ConfOption` in `ScreenOptionsMasterPrefs.cpp`, the credits overlay in
`ScreenSystemLayer.cpp`, the debug-overlay cycle in
`ScreenDebugOverlay.cpp`. There was a real upstream saga to remove Pay
mode once (`CoinMode_Pay->CoinMode_Free, step 1.` and its reverts),
but every one of those commits — including the final "Add Pay mode
back" ones — already predates this fork (`git merge-base --is-ancestor`
confirms). **The actual reason Pay mode looks inert**:
`PrefsManager.cpp`'s `m_bEventMode` preference defaults to `true`
(`git blame`: this default is from `a085d0d1da6`, 2011-03-17, genuine
upstream StepMania behavior, not fork-specific), and
`GameState::GetCoinMode()` silently downgrades `CoinMode_Pay` to
`CoinMode_Free` whenever `IsEventMode()` is true. So picking "Pay" in
System Options has no visible effect until `EventMode` is *also*
turned off (a separate preference, not exposed in the same 3-way
selector). Confirmed and pinned with a new characterization test,
`tests/test_GameState.cpp` (registered in `tests/CMakeLists.txt`,
`[GameState][CoinMode]` tag), covering Home/Free (never charge),
Pay-with-EventMode-on (silently downgrades to Free — the surprising
case), and Pay-with-EventMode-off (correctly charges
`CoinsPerCredit` and gates `EnoughCreditsToJoin()` on `m_iCoins`). No
production code changed for this — maintainer asked only to confirm
it works, not to change the default. If a different default (Pay
without needing to separately flip EventMode) is ever wanted, that's
its own follow-up decision.

### 10. RString everywhere
`typedef StdString::CStdString RString` (`global.h:107`), 723 files /
~8,429 uses. Declared retirement goal (ADR 0001 Settled #5).
**Action:** [`playbooks/migrate-rstring.md`](./playbooks/migrate-rstring.md),
per subsystem, opportunistic.

**Pilot #1 (2026-09-12): `Grade.cpp`/`Grade.h`.** Smallest possible
leaf subsystem (10 total `RString` mentions) to validate the playbook's
boundary-safety claim in practice. `GradeToString` (header-inline),
`GradeToOldString`, `GradeToLocalizedString` (return `RString`→
`std::string`) and `StringToGrade` (param `const RString&`→
`const std::string&`) migrated; `StringToGrade`'s local
`s.MakeUpper()` (a `CStdString`-only method, not on `std::string`)
replaced with the `RageUtil` free function
`if(!s.empty()) MakeUpper(&s[0], s.size())`, mirroring `CStdStr::MakeUpper()`'s
own implementation (`StdString.h`) which does the identical empty-guard
+ in-place-buffer call. Confirmed via `StdString.h:361`
(`CStdStr(const std::string& str): MYBASE(str)`) that the boundary
works in **both** directions with zero call-site changes needed: a
`std::string` implicitly converts to `RString` (converting ctor) when
passed to or assigned into not-yet-migrated code, and an `RString`
implicitly satisfies a `const std::string&` parameter (`CStdString`
derives from `std::basic_string<char>`). 8 caller files
(`GradeDisplay.cpp`, `HighScore.cpp`, `NetworkSyncManager.cpp`,
`PlayerStageStats.cpp`, `Profile.cpp`, `ScreenEvaluation.cpp`,
`SongUtil.cpp`, `Grade.h`'s own macro users) all compiled unchanged.
Verified: `sm_tests` 5966/226 unchanged, `[Grade]` characterization tag
38/4 unchanged, `ctest` 100%, Release `StepMania-R.exe` clean rebuild,
`--SelfTest` exit 0. **Playbook's core claim validated — safe to
continue picking small leaf subsystems opportunistically.**

**Pilot #2 (2026-09-12): `Command.cpp`/`Command.h`.** Second small
leaf subsystem (~20 total `RString` mentions). `Command::GetName()`,
`Command::GetOriginalCommandString()`, `Commands::GetOriginalCommandString()`,
and the `Command::Arg::s` member all migrated `RString`→`std::string`.
Internal storage (`m_vsArgs` as `std::vector<RString>`, `Load()`'s and
`ParseCommands()`'s `const RString&` parameters, and the `split()`/`join()`
calls) deliberately **left as `RString`** — `RageUtil`'s `split`/`join`
take `std::vector<RString>&`/`const std::vector<RString>&`, and
`std::vector<Derived>` has no relationship to `std::vector<Base>` (no
container covariance), so migrating the vector's element type would
require migrating those `RageUtil` signatures too — out of scope for
one bounded subsystem pass, exactly the "big boundary, pick a
different subsystem" case the playbook warns about. Scalar-value
boundaries (a single `RString`/`std::string` copy, not a container)
stayed safe throughout, confirmed via `StdString.h:361`'s converting
constructor.
**Found one real hard boundary** (not just "verify it compiles" —
an actual blocker): `Difficulty.h`'s `StringToDifficulty(const RString&)`
takes its argument by **reference to the derived type**, which a
plain `std::string` argument cannot bind to (only the reverse
direction — derived-to-base reference binding — is implicit). Its two
call sites in `UnlockManager.cpp` (`e.m_cmd.GetArg(1).s` now being
`std::string`) needed an explicit `RString(...)` wrap. This is the
converse of the direction the playbook calls out as safe, so it's now
recorded as a distinct gotcha in the playbook. All other boundary
calls in `UnlockManager.cpp`/`OptionRowHandler.cpp`/`GameCommand.cpp`/
`LuaManager.cpp` (by-value `RString` parameters, `const std::string&`
parameters like `StringToInt`, `RString` variable assignment,
`.c_str()`) needed no changes — implicit conversion handled them.
Verified: `sm_tests` 5966/226 unchanged, `[Command]` characterization
tag 26/6 unchanged, `ctest` 100%, Release `StepMania-R.exe` clean
rebuild, `--SelfTest` exit 0.

**Pilot #3 (2026-09-12): `ScoreDisplayCalories.cpp`/`.h`.** The file's
single `RString` member, `m_sMessageOnStep`, migrated to `std::string`.
Notable: grepping the whole file shows this member is declared and
checked (`!m_sMessageOnStep.empty()`) but **never actually assigned
anywhere in the class** — it stays default-constructed (empty) for the
lifetime of every instance, so the `MESSAGEMAN->Unsubscribe(...)`
branch that reads it never executes in practice. Pre-existing
dead-ish state, unrelated to this migration and left as-is (not a
parsing/behavior bug to fix here). `MessageManager::Unsubscribe(
IMessageSubscriber*, const RString&)` takes its second argument by
reference-to-derived-type — the same hard-boundary gotcha from pilot
#2 — so the one call site got the same `RString(...)` wrap, this time
self-contained within the same file (no other file needed a
caller-side fix). **Also scouted and explicitly rejected as pilots**:
`MeterDisplay.cpp`'s `Load(RString,float,RString)` and
`ComboGraph.cpp`'s `Load(RString)` both immediately forward their
by-value `RString` parameters into `AutoActor::Load(const RString&)` /
`ThemeManager`/`ThemeMetric` APIs — exactly the "big boundary, pick a
different subsystem" case the playbook already warns about, since
`AutoActor`/`ThemeManager` are large un-migrated subsystems in their
own right and wrapping every call site there wouldn't meaningfully
shrink the codebase's real `RString` footprint. Similarly
`PlayerAI.cpp`'s lone `RString sKey` feeds directly into
`IniFile`/`XNode::GetChild()`, explicitly named in the playbook's
existing "Serialization / IniFile / XmlFile ... heavy RString users"
warning — skipped for the same reason. **New lesson: a low `RString`
mention *count* isn't sufficient to pick a pilot — check what each
mention actually touches; immediate pass-through into a large
un-migrated subsystem (even via just 1-2 lines) is a bad pilot
regardless of how small the file itself is.**
Verified: `sm_tests` 5966/226 unchanged (no dedicated characterization
test exists for this widget), `ctest` 100%, Release `StepMania-R.exe`
clean rebuild, `--SelfTest` exit 0.

**Scouting pause (2026-09-13):** re-scouted ~15 more small candidates
(`Bookkeeper.cpp`, `CourseContentsList.cpp`, `LuaExpressionTransform.cpp`,
`ModsGroup.h`, `ScrollBar.cpp`, `ActiveAttackList.cpp`,
`BeginnerHelper.cpp`, `CryptHelpers.h`, `GradeDisplay.cpp`,
`OptionsCursor.cpp`, `DancingCharacters.cpp`) file-by-file, not just by
`RString` grep count. Every one either passes its `RString` straight
into a large un-migrated subsystem (`ThemeManager`/`AutoActor`/
`IniFile`/`XNode`/`PlayerOptions`) or hits a genuine hard boundary not
safe to paper over: `LuaExpressionTransform.cpp`'s `error` local feeds
`LuaManager.h`'s `RunScriptOnStack(Lua*, RString &Error, ...)`, a
**non-const reference out-param** — wrapping it in a temporary
`RString(...)` copy would silently discard whatever the callee writes
back through the reference, an actual behavior risk, not a style
objection. **The opportunistic small-leaf-file lane is exhausted for
now** — the 3 pilots above (`Grade`, `Command`, `ScoreDisplayCalories`)
stand as the session's contribution; further progress needs either a
bigger bounded subsystem pass (done when already working there for
another reason, per the playbook's own "when to use" guidance) or
waiting for new small candidates to surface. Do not re-scout the same
~25 files without new information.

**Pilot #4 (2026-09-13, autonomous — "completa el backlog solo con lo
no bloqueante" goal): `StyleUtil.h`/`.cpp`'s `StyleID` class.** Found
by widening the scouting net to 2-3-mention files and specifically
looking for **private members** — `StyleID::sGame`/`sStyle` are
private, giving zero external call-site exposure by construction (only
`StyleUtil.cpp`'s own methods touch them). Migrated both to
`std::string`. All but one call qualified as boundary-safe by the
established rules: `Game::m_szName`/`Style::m_szName` are `const
char*` (assigns fine either way), `GAMEMAN->StringToGame`/
`GameAndStringToStyle` take `RString` **by value** (implicit
conversion), `XNode::AppendAttr(const RString&, T value)` takes its
value **by value** too (implicit conversion into the eventual
`XNodeValue::SetValue(const RString&)` call), and `operator<`'s string
comparisons are native to `std::string`.
**Found a new, more subtle hard-boundary shape**:
`XNode::GetAttrValue(const RString &sName, T &out)` is a *template* on
`T`, so it looks generic — but it forwards to
`XNodeValue::GetValue(T &out)`, which is **not actually templated**:
`XNodeValue` only declares four concrete virtual overloads
(`GetValue(RString&)`/`(int&)`/`(float&)`/`(bool&)`/`(unsigned&)`), so
instantiating the outer template with `T = std::string` fails to find
a matching virtual overload — a compile error, not a silent behavior
change, but still a real blocker a template's outer generic-looking
signature can hide. Fixed in `StyleID::LoadFromNode()` with a local
`RString` temporary (loaded via the working `RString&` overload, then
assigned into the `std::string` member — the assignment direction is
always safe). **New lesson: a templated wrapper function isn't proof
the underlying call is generic — check what the template's body
actually calls, especially for anything backed by a non-template
virtual dispatch (this codebase's `XNodeValue` is a common one to
watch for, since `IniFile`/`XmlFile` are both built on it).**
Verified: `sm_tests` 5981/230 unchanged (no dedicated characterization
test exists for `StyleID`), `ctest` 100%, Release `StepMania-R.exe`
clean rebuild, `--SelfTest` exit 0.

**Pilot #5 (2026-09-13, autonomous — same standing goal):
`CourseUtil.h`/`.cpp`'s `CourseID` class.** Same private-member
scouting heuristic as pilot #4, one level harder: `CourseID` has a
**public getter that returns a reference to the private member**
(`const RString &GetPath() const { return sPath; }`), which is NOT
just an internal-boundary case — a `const RString&` reference to a
`std::string` member can't be formed (base object, derived reference
type), so this getter needed to change too. Fixed by dropping the
reference (`RString GetPath() const { return sPath; }`, returning a
by-value temporary instead) rather than changing the return type to
`const std::string&` — this is only called from one place in the whole
codebase (`Profile.cpp:2144`, `splitpath(courseID.GetPath(), ...)`,
where `splitpath`'s first param is `const RString&`), and a by-value
`RString` return binds to that directly with **zero caller-side
changes needed**, matching the general principle "prefer the fix that
needs no external changes when the call site is this rare." Also hit
two `.Left(n)` calls (an `RString`-only method with no `std::string`
equivalent) — replaced with `.substr(0, n)` per the playbook's
existing mapping table; confirmed via `StdString.h:500`
(`CStdStr::Left` is itself implemented as a clamped `substr(0, n)`)
that this is an exact behavioral match, not an approximation. Hit the
same `XNode::GetAttrValue` hard-boundary shape as pilot #4 in
`LoadFromNode()`, fixed the same way (local `RString` temporaries).
`CourseID` is §5-adjacent (`.crs` course-file identity/loading) —
re-verified the `[crs]` characterization tag before and after: 39/5,
unchanged, matching the documented baseline exactly.
Verified: `sm_tests` 5981/230 unchanged, `[crs]` 39/5 unchanged,
`ctest` 100%, Release `StepMania-R.exe` clean rebuild, `--SelfTest`
exit 0.

**Pilot #6 (2026-09-13, autonomous — same standing goal):
`SongUtil.h`/`.cpp`'s `SongID` class.** Same private-member scouting
vein: checked `SongUtil.h`/`StepsUtil.h`/`TrailUtil.h`'s ID classes
together. `TrailID` turned out to have **zero `RString` members at
all** (just `StepsType`/`CourseDifficulty`) — nothing to migrate.
`StepsID`'s `sDescription` is private with no reference-returning
getter (`GetStepsType()`/`GetDifficulty()` only expose the non-string
fields) — a clean future pilot, not done this round. `SongID::sDir`
(private, no exposing getter either) migrated to `std::string` —
identical shape to `CourseID`: one `.Left(1)`/`.Left(16)` pair replaced
with `.substr(0, 1)`/`.substr(0, 16)`, the same `XNode::GetAttrValue`
hard-boundary fix via a local `RString` temporary in `LoadFromNode()`,
`Song::GetSongDir()`/`SongManager::GetSongFromDir(RString)` both
qualify as boundary-safe under the established rules. `SongID` is
§5-adjacent (core song identity/loading) — re-verified the `[corpus]`
characterization tag before and after: 313/3, unchanged, matching the
documented baseline exactly.
Verified: `sm_tests` 5981/230 unchanged, `[corpus]` 313/3 unchanged,
`ctest` 100%, Release `StepMania-R.exe` clean rebuild, `--SelfTest`
exit 0.

**Pilot #7 (2026-09-13, autonomous — same standing goal):
`StepsUtil.h`/`.cpp`'s `StepsID::sDescription`.** Closes out the whole
`*ID` family scouted this round. Two hard boundaries this time, not
one: the usual `XNode::GetAttrValue` shape in `LoadFromNode()` (local
`RString` temporary, same fix as every other `*ID` class), plus a new
one — `SongUtil::GetOneSteps(...)`'s `sDescription` parameter is
`const RString &sDescription` (a genuine reference parameter, not
by-value like `StringToGame`/`GetCourseFromName`/etc. seen in the
earlier `*ID` pilots) — fixed with an explicit `RString(sDescription)`
wrap at the one call site in `ToSteps()`. Everything else qualified as
safe under the established rules: `Steps::GetDescription()` returns
`RString` by value (assigns into the `std::string` member fine),
`AppendAttr`'s by-value template parameter, and the `operator<`/
`operator==` `COMP()` macros' native `std::string` comparisons.
§5-adjacent (Steps/chart identity) — re-verified `[corpus]` (313/3)
unchanged before/after, on top of the usual gate.
Verified: `sm_tests` 5981/230 unchanged, `[corpus]` 313/3 unchanged,
`ctest` 100%, Release `StepMania-R.exe` clean rebuild, `--SelfTest`
exit 0.

**Pilot #8 (2026-09-13, autonomous — same standing goal):
`CryptHelpers.h`/`.cpp`'s `RSAKeyWrapper::Load(...)`.** A different
shape than every earlier pilot: not a private data member, but a
**public method's own parameter types** — `bool Load( const RString
&sKey, RString &sError )`. Migrating a function signature (rather than
internal storage behind an unchanged interface) is usually a bigger,
more externally-visible change, but this one stayed small: only 4
call sites, all inside `CryptManager.cpp` itself. Changed the
signature to `const std::string &sKey, std::string &sError` — **zero
caller-side changes needed**, because `RString`'s inheritance from
`std::basic_string<char>` means an existing `RString` argument/out-
param variable at each call site binds directly to the new
`const std::string&`/`std::string&` parameter types (the safe "derived
object satisfying a base-type reference parameter" direction, just
applied to a function's own declared parameter types instead of an
internal member). Confirmed via a rebuild that only recompiled 2 files
(`CryptHelpers.cpp`, `CryptManager.cpp`) — proof the change is exactly
as contained as expected, no unexpected header-inclusion cascade.
`Load()`'s body only used `sKey.data()`/`.size()` (native) and assigned
`error_to_string(iRet)`'s result into `sError` (safe either direction).
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #9 (2026-09-13, autonomous — same standing goal):
`ScoreKeeper.h`/`.cpp`'s `MakeScoreKeeper(...)` factory method.**
Another public-function-signature migration like pilot #8, this time a
**by-value parameter** (`RString sClassName` → `std::string sClassName`)
rather than a reference — even simpler, since a by-value parameter
accepts either an `RString` or `std::string` argument at the call site
with zero ambiguity either way. Only 1 external caller
(`ScreenGameplay.cpp:169`). Body only compares `sClassName == "..."`
(native `std::string`/string-literal comparison) and calls `.c_str()`
(native) — no hard boundaries at all.
**Also scouted `Trail.h`'s public `Modifiers` member and deliberately
did not migrate it this round** — unlike the private `*ID`-class
members, `Modifiers` is touched directly from 3 external files
(`Course.cpp`, `CourseContentsList.cpp`, `GameState.cpp`), and one of
those calls, `PlayerOptions::FromString(const RString &sMultipleMods)`,
takes a real reference parameter (hard boundary, would need a wrap).
Worse, `Message::SetParam(const RString&, const T&)` is a template that
forwards to `LuaHelpers::Push(L, val)` — potentially the same
"templated wrapper, non-template backing" trap found migrating
`StyleUtil`/`XNode::GetAttrValue` — not yet confirmed either way.
Flagged as a real candidate for a future round once
`LuaHelpers::Push`'s overload set is checked, not attempted half-sure.
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #10 (2026-09-13, autonomous — same standing goal): the
`Trail::Modifiers` candidate flagged in pilot #9, unblocked.**
Checked `LuaHelpers::Push`'s actual overload set (`LuaManager.cpp:93`)
and found it **already has a real `std::string` specialization**
(`template<> void Push<std::string>(...)`, alongside the `RString`
one) — unlike `XNode::GetAttrValue`, this template genuinely is
generic for both string types, so `Message::SetParam(const RString&,
const T&)` calling it is safe. Migrated `TrailEntry::Modifiers`
(a public field, per the earlier scouting) to `std::string`. Needed
**3 explicit `RString(...)` wraps**, not one: two hard-reference-
parameter calls inside `Trail.cpp` itself
(`Attack::FromGlobalCourseModifier(const RString&)` in
`GetAttackArray()`, `PlayerOptions::FromString(const RString&)` in
`ContainsTransformOrTurn()`), plus the same `PlayerOptions::FromString`
call from the external caller in `GameState.cpp`
(`GetAllUsedNoteSkins()`). The other two external touch points
(`Course.cpp`'s `te.Modifiers = e->sModifiers;` — `RString` assigned
into the member, safe direction; `CourseContentsList.cpp`'s
`msg.SetParam("Modifiers", te->Modifiers)` — now confirmed safe via
the `Push<std::string>` specialization) needed no changes at all.
§5-adjacent (course/trail loading) — re-verified both `[corpus]`
(313/3) and `[crs]` (39/5) unchanged before/after.
Verified: `sm_tests` 5981/230 unchanged, `[corpus]` 313/3 unchanged,
`[crs]` 39/5 unchanged, `ctest` 100%, Release `StepMania-R.exe` clean
rebuild, `--SelfTest` exit 0.

**Scouting bug found immediately after (2026-09-13, no code
committed):** attempted `CommandLineActions::CommandLineArgs::argv`
next, scouted with a `src/*.cpp`-only grep (misses subdirectories),
missed `src/archutils/Win32/GraphicsWindow.cpp` passing it into a
`std::vector<RString>&` parameter — the known container hard-boundary.
The real build caught it before any commit; reverted, net zero diff.
**Standing fix: scout with a genuinely recursive `src/` search from
now on, no single-directory glob.** See `playbooks/migrate-rstring.md`
for the full writeup.

**Pause point (2026-09-13): checked `src/arch/`/`src/archutils/` with
the corrected recursive method — not a fruitful vein.** Nearly every
small candidate there is a Linux/Mac-specific driver file (out of
scope per `AGENTS.md` §3's Windows-first priority, and mostly
impossible to compile-test on Windows anyway). The one cross-platform
candidate, `arch/RageDriver.h`'s `DriverList::Create(const RString&)`,
is foundational driver-registry infrastructure likely called from many
per-platform driver `.cpp` files — too wide-reaching for a quick pilot.
**After 10 pilots, the easy/quickly-scoutable RString vein in `src/`
is genuinely thin now.** Further progress needs either a deliberately
bigger bounded subsystem pass (per the playbook's own "when to use"
guidance — while already working there for another reason, not as a
cold scouting exercise) or fresh small candidates surfacing later.

**Pilot #11 (2026-09-13, found while scouting `GameState.h` for item 9
clusters): `GameState::m_RandomAttacks`.** A `std::vector<RString>`
public member migrated to `std::vector<std::string>` — a direct
type change, not a god-object "carve into a component" cluster (no
reference-member technique needed, since a plain member-type swap
works fine when nothing outside the class needs a reference bound to
it). Only 2 external files touch it (`Player.cpp`, `SongManager.cpp`),
all via native container methods (`.empty()`/`.size()`/`operator[]`/
`.clear()`/`.push_back()`) — no function anywhere takes the whole
vector by reference to an un-migrated `vector<RString>&` parameter
(the `Command.cpp`/`CommandLineActions` container trap), so this one
had zero hard boundaries. `Player.cpp`'s `ApplyRandomAttack()` returns
`RString` from an `operator[]` read — safe (implicit `std::string`→
`RString` conversion on return); `SongManager.cpp`'s
`.push_back(sAttack)` passes an `RString sAttack` local — safe (upcast
binding to the `push_back(const std::string&)` parameter).
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #12 (2026-09-13, item 9's low-risk clusters exhausted, pivoted
back to item 10): `PlayerStageStats::FormatPercentScore(float)`.** A
static function's own return type migrated `RString` → `std::string`.
Body only builds the result via `ssprintf(...)` (already safe:
`std::string s = ssprintf(...)` slices the returned `RString`'s
`std::basic_string<char>` base, no `RString`-specific data lost). Only
1 real external caller (`PaneDisplay.cpp`'s
`sTextOut = FormatPercentScore(...)`, where `sTextOut` is `RString&`) —
safe because `CStdStr` has a genuine
`operator=(const std::string&)` overload (`StdString.h:391`), not just
the inherited base-class one. The `LuaFunction(FormatPercentScore,
...)` macro registration forwards through `LuaHelpers::Push(L, expr)`,
already confirmed generic for both `RString`/`std::string` in pilot
#10. Verified: `sm_tests` 5981/230 unchanged (rebuild recompiled only
the ~40 files that include `PlayerStageStats.h`, no wider cascade),
`ctest` 100%, Release `StepMania-R.exe` clean rebuild, `--SelfTest`
exit 0.

**Pilot #13 (2026-09-13): `InputQueueCodeSet::Load(...)`'s parameter
and `::Input(...)`'s return type (`CodeSet.h`/`.cpp`).** `Load`'s
`const RString&` parameter and `Input`'s `RString` return both migrated
to `std::string`. `m_asCodeNames` (`std::vector<RString>`) stays
`RString` — it feeds `split(const RString&, const RString&,
std::vector<RString>&, bool)` (`RageUtil.h:406`), which has no
`vector<std::string>&` overload, the same container hard-boundary as
`CommandLineActions::argv`/`Command.cpp`'s `m_vsArgs`. `Load`'s body
calls `THEME->GetMetric(const RString&, const RString&)` through the
`CODE_NAMES`/`CODE(s)` macros — a real reference-parameter hard
boundary into the still-`RString` `ThemeManager` — fixed by wrapping
`sType` in `RString(...)` inside both macro definitions (2 wraps, one
per macro, both localized to `CodeSet.cpp`). External callers
(`OptionsList.cpp`'s `m_Codes.Load(sType)`, `Screen.cpp`'s
`m_Codes.Load(m_sName)`) both pass an `RString` local — safe, binds to
`const std::string&` with zero changes. `InputMessage`'s internal
`sCodeName` local also switched to `std::string` (purely internal,
feeds `Message::SetParam`, already confirmed generic in pilot #10).
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #14 (2026-09-13): `RandomSample`'s `Load`/`LoadSoundDir`/
`LoadSound` parameters (`RandomSample.h`/`.cpp`).** All three by-value
`RString` parameters migrated to `std::string`. Fixed 5 hard-boundary
calls into still-`RString` `RageUtil` free functions:
`GetExtension(const RString&)` (1 wrap) and
`GetDirListing(const RString&, std::vector<RString>&, ...)` (4 wraps,
one per extension) — the output vector (`arraySoundFiles`) stays
`std::vector<RString>` since `GetDirListing` fills it by reference.
Replaced the RString-only `.Right(1)` with `.substr(sDir.size()-1)`
per the playbook's mapping table (safe here since `sDir` is already
known non-empty at that point). `RageSound::Load(RString sFile)`
(by-value) needed no wrap — passing `std::string` into an `RString`
by-value parameter is the established safe direction. Only 1 external
caller (`ScreenSelectMaster.cpp`'s `m_soundDifficult.Load(
ANNOUNCER->GetPathTo(...))`, which returns `RString` by value) needed
zero changes. Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%,
Release `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #15 (2026-09-13): `RageWorkerThread`'s constructor parameter
and `m_sName` member (`RageUtil_WorkerThread.h`/`.cpp`).** Private
member, no exposing getter — only used with `.c_str()` in
`LOG_TRACE`/`LOG_WARN`, works for either type. Zero wraps needed: the
constructor body builds `RageEvent` names via
`"\"" + sName + "\" worker event"` (`std::string` arithmetic, still
produces `std::string`), which then binds into
`RageEvent(RString name)`'s **by-value** parameter — the established
safe direction, no `RString(...)` wrap required. Both external
subclass callers (`MemoryCardManager.cpp`'s string-literal argument,
`RageFileDriverTimeout.cpp`'s `ThreadedFileWorker`'s own by-value
`RString sPath` forwarded straight through) needed zero changes.
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #16 (2026-09-13): `RageSurfaceUtils::SaveSurface`/
`LoadSurface`'s by-value `file` parameter (`RageSurfaceUtils.h`/
`.cpp`).** Both migrated `RString` → `std::string`. `RageFile::Open(
const RString&, int)` is a real reference-parameter hard boundary into
the still-`RString` `RageFile` — fixed with 2 `RString(file)` wraps,
one per function, both local to `RageSurfaceUtils.cpp`. Only 1
external caller file (`ImageCache.cpp`, 3 call sites), all passing a
`const RString sCachePath` local — safe, binds to the by-value
`std::string` parameter with zero changes. Verified: `sm_tests`
5981/230 unchanged, `ctest` 100%, Release `StepMania-R.exe` clean
rebuild, `--SelfTest` exit 0.

**Pilot #17 (2026-09-13, found via a broad scouting fork after item 9's
low-risk clusters and item 10's small-header candidates both thinned
out): `FontManager::LoadFont(...)`'s parameters
(`FontManager.h`/`.cpp`).** Both migrated `RString` → `std::string`
(`const RString&` → `const std::string&`, by-value `RString` → by-value
`std::string`). One hard boundary: `Font::Load(const RString&,
RString)` (`Font.h:176`) — fixed with 1 `RString(...)` wrap on the
first argument; the second (`sChars`) is by-value and binds with no
wrap. `FontName` (`typedef std::pair<RString,RString>`, used as the
font cache's map key) needed no change — `std::pair`'s templated
constructor accepts `std::string` arguments converting implicitly to
each `RString` field. Only 2 external callers, both in
`BitmapText.cpp` (`LoadFromFont`/`LoadFromTextureAndChars`), both
passing `const RString&` locals — safe, zero changes. Verified:
`sm_tests` 5981/230 unchanged, `ctest` 100%, Release `StepMania-R.exe`
clean rebuild, `--SelfTest` exit 0. **A dedicated scouting pass (fork)
confirmed backlog items #12 (mechanical clang-tidy debt — blocked:
`clang-tidy` isn't even installed on this machine, plus the §5/
`clang-format`/maintainer-flagged blockers already on record), #15
(`#if 0` dead blocks — remaining ~7 sites need live gameplay/editor
reasoning), #26 (binary blobs — already fully worked through, only
maintainer-gated remainders left), and #29 (orphaned SMOnline
networking — explicitly maintainer-deferred) are all genuinely
maintainer-gated, not just under-scouted.**

**Item 9 investigated for other targets beyond `GameState` (2026-09-13):
`Player`/`ScreenGameplay` checked and rejected as split targets for
now.** Both are listed in this item's own oversized-file table, but
neither has `GameState`'s crisp safety prerequisite — an already
deleted/private copy constructor and assignment operator. `GameState`'s
were confirmed private/undefined before any cluster work started
(making the reference-member technique provably safe: no code path can
ever copy a `GameState` and alias its component's storage).
`Player`/`ScreenGameplay` declare no such override; `Player` derives
from `Actor`, which has a virtual `Copy()` clone method that `Player`
does not override, so whether a `Player` is ever actually copied
polymorphically is genuinely ambiguous rather than provably impossible.
Applying the reference-member technique to an object that *can* be
copied would silently alias two objects' "component" storage instead
of giving each its own — exactly the kind of subtle bug the technique's
prerequisite exists to rule out. Not pursued without either (a) proof
no copy path exists, or (b) first adding an explicit deleted copy
ctor/assignment to `Player`/`ScreenGameplay` as its own separate,
verifiable safety-hardening change.

**Pilot #18 (2026-09-13): `Inventory.cpp`'s file-local `Item::sModifier`
field.** `struct Item` is declared entirely inside `Inventory.cpp`
(not in any header) — zero external exposure by construction, the
cleanest possible shape. Both touch points are safe-direction
assignments already established this session:
`item.sModifier = ITEM_EFFECT(i)` (RString-returning macro assigned
into std::string, safe) and `a.sModifiers = g_Items[...].sModifier`
(std::string assigned into `Attack::sModifiers`, still `RString` —
safe via `CStdStr::operator=(const std::string&)`, `StdString.h:391`).
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #19 (2026-09-13): `ProfileManager.cpp`'s file-local
`DirAndProfile::sDir` field.** Found via a new scouting technique
(grepping for file-local `.cpp`-only `struct` bodies containing an
`RString` field — same shape as pilot #18). `Profile::LoadTypeFromDir`/
`LoadAllFromDir`/`LoadEditableDataFromDir`/`SaveTypeToDir`/
`HandleStatsPrefixChange` (`Profile.h:403-420`) are all **by-value**
`RString` parameters, so every `derp.profile.LoadTypeFromDir(derp.sDir)`
-style call needed zero changes (safe direction). Two real hard
boundaries: a local `const RString &sOther = dap.sDir;` reference
binding, fixed by changing its declared type to
`const std::string &` (not a wrap — a derived-reference-to-base-object
binding is simply invalid, unlike the reverse); and two
`LocalProfileDirToID(const RString&, ...)` calls passing a migrated
`.sDir` field, fixed with `RString(...)` wraps. `struct DirAndProfile`
lives entirely in `ProfileManager.cpp`, never in a header — zero
external exposure. Verified: `sm_tests` 5981/230 unchanged, `ctest`
100%, Release `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #20 (2026-09-13): `InputFilter.cpp`'s `ButtonState::m_sComment`
field.** `struct ButtonState` is only *forward*-declared in
`InputFilter.h` (`struct ButtonState;`) and used solely as a reference
parameter (`CheckButtonChange(ButtonState &bs, ...)`) — its full
definition, and every touch of `m_sComment`, lives entirely in
`InputFilter.cpp`. The two real public entry points
(`InputFilter::GetButtonComment()`/`SetButtonComment()`,
`InputFilter.h:74`) keep their `RString` signatures unchanged — the
getter's `return GetButtonState(di).m_sComment;` implicitly
constructs an `RString` from the migrated `std::string` field on
return, and the setter's `bs.m_sComment = sComment` assigns an
`RString` parameter into the `std::string` field — both already-
established safe directions, so **zero external callers of
`Get`/`SetButtonComment` needed any change** despite the internal
storage migrating. Verified: `sm_tests` 5981/230 unchanged, `ctest`
100%, Release `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #21 (2026-09-13): `PaneDisplay.cpp`'s file-local
`Content_t::sFontType` field.** `struct Content_t` and its
`static const` aggregate-initialized array (`g_Contents[]`, string
literals like `"count"`) both live entirely in the `.cpp` — zero
external exposure. Only 1 touch point:
`RString sFontType = g_Contents[pc].sFontType;` — a local variable
deliberately kept as `RString` (unrelated to the migration), so this
line already just implicitly constructs an `RString` from the
migrated `std::string` field (safe direction) and the following
`THEME->GetPathF(sMetricsGroup, sFontType)` call needed **no wrap at
all**, since it's operating on the still-`RString` local, not the
struct field directly. Verified: `sm_tests` 5981/230 unchanged,
`ctest` 100%, Release `StepMania-R.exe` clean rebuild, `--SelfTest`
exit 0.

**Pilot #22 (2026-09-13): `RageFileManager.cpp`'s file-local
`LoadedDriver::m_sType`/`m_sRoot`/`m_sMountPoint` fields.** The
biggest single-file pilot yet (27 touch points across 3 fields), but
fully mechanical and self-contained — `LoadedDriver` is never named in
`RageFileManager.h`. New confirmed-safe/fixable shapes: (1)
`.CompareNoCase(...)` is a `CStdStr`-only facade method with **no
direct `std::string` equivalent**, but its own implementation
(`StdString.h:492`) is just `ssicmp(this->c_str(), szThat)` —
`StdString::ssicmp` is a plain templated free function usable directly
on two `.c_str()` results, so every `.CompareNoCase(x)` call becomes
`StdString::ssicmp(a.c_str(), x.c_str())` with byte-identical behavior
(it's literally the same function the method already called
internally) — note the `StdString::` qualifier is required, `ssicmp`
lives in that namespace, not global scope (first build attempt failed
with `C3861: 'ssicmp': identifier not found` until this was added).
(2) A local `const RString &mountPoint = pLoadedDriver->m_sMountPoint;`
reference binding needed retyping to `const std::string&` (same
derived-reference-to-base-object shape as pilot #19's `sOther`,
cascading into a second dependent local, `trimPoint`, which also
needed retyping). `DriverLocation` (`RageFileManager.h:64-67`, a real
public-API struct returned by `GetLoadedDrivers()`) was deliberately
**left as `RString`** — assigning the migrated fields into its
still-`RString` members is the established safe direction, so the
public API's own type didn't need to change. Since this underlies all
file I/O including song loading, re-verified `[corpus]` (313/3) and
`[crs]` (39/5) as an extra precaution even though `LoadedDriver` itself
isn't part of the simfile parse path. Verified: `sm_tests` 5981/230
unchanged, `ctest` 100%, Release `StepMania-R.exe` clean rebuild,
`--SelfTest` exit 0.

**Pilots #23-24 (2026-09-13, two more file-local structs found via the
same technique): `ScreenInstallOverlay.cpp`'s
`PlayAfterLaunchInfo::sSongDir`/`sTheme` and `StepMania.cpp`'s
`VideoCardDefaults::sDriverRegex`/`sVideoRenderers`.** Both structs
live entirely in their own `.cpp`, neither named in the matching
header. `PlayAfterLaunchInfo` needed zero hard-boundary fixes at all
— every touch (`SongManager::GetSongFromDir(RString)` by-value, field-
to-field assignment inside `OverlayWith()`) was already a safe
direction. `VideoCardDefaults` needed two: `Preference<RString>::
Set(const T&)` (`defaults.sVideoRenderers` wrapped in `RString(...)`)
and another `.CompareNoCase(...)` call, fixed with the same
`StdString::ssicmp(a.c_str(), b.c_str())` substitution pilot #22
established. Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%,
Release `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #25 (2026-09-13): `RageFileDriverMemory.cpp`'s file-local
`RageFileObjMemFile::m_sBuf` field.** An in-memory byte-buffer used by
`RageFileObjMem` (the "memory filesystem" file object). One real
public API touch: `RageFileObjMem::GetString()`
(`RageFileDriverMemory.h:29`) previously returned `const RString&` —
can't keep returning a reference once the field is `std::string` (the
established pilot #5/#19/#22 "derived-reference-to-base-object"
shape), so it changed to return `RString` **by value** instead —
requires **zero caller changes anywhere**, since every caller either
assigns the result into an `RString`/`RString&` variable or passes it
as a template argument that still deduces `RString` either way (both
already-safe patterns). `PutString(const RString&)` assigns an
`RString` into the `std::string` field — safe direction, no change.
`.replace(...)`/`operator[]`/`.size()` are all inherited unmodified
from `std::basic_string` in *both* `RString` and `std::string` — never
RString-specific overrides — so their behavior is byte-for-byte
identical regardless of the field's declared type. Since this class is
used by `CourseWriterCRS.cpp` (writing `.crs` course files, a §5
content-compatibility format) and `XmlFileUtil.cpp`/
`RageFileDriverDeflate.cpp` (profile/stats serialization), re-verified
both `[crs]` (39/5) and `[corpus]` (313/3) as an extra precaution.
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #26 (2026-09-13): `NoteSkinManager.cpp`'s
`NoteSkinData::sName` field.** `struct NoteSkinData` is forward-
declared and used only by reference (`NoteSkinData& data_out`) in
`NoteSkinManager.h` — same shape as pilot #20's `ButtonState`.
`vsDirSearchOrder` (`std::vector<RString>`) deliberately left
untouched (a container, not this pilot's scope). Only 3 real touch
points: an assignment from a `const RString&` parameter (safe), a
`push_back` into a `vector<RString>&` out-parameter (safe, implicit
`RString` construction), and one hard boundary —
`LuaHelpers::ReportScriptError(RString const&, ...)` receiving a
string-concatenation expression that now evaluates to `std::string`
once `sName` migrates — fixed with a single `RString(...)` wrap around
just the `data_out.sName` term (cheaper than wrapping the whole
concatenation). Rejected in the same sweep:
`RageSoundReader_WAV.cpp`'s `WavReader::m_sError` — genuinely
infeasible without a much larger blast radius, since
`FileReading::read_8`/`read_16_le` (`RageFile.h:106-107`) take a
**mutable** `RString&` out-parameter used broadly elsewhere
(`RageFileDriverDeflate.cpp`, `RageFileDriverZip.cpp`), a hard boundary
with no cheap per-call fix. `XmlToLua.cpp`'s `actor_template_t`
(8+ RString fields, a `std::map<RString,RString>` member, 6+ methods,
recursive `std::vector<actor_template_t>`) is real but sized more like
its own dedicated pilot than a quick win — deferred, not rejected.
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**Pilot #27 (2026-09-13): `ThemeManager.cpp`'s `Theme::sThemeName` and
`CompareLanguageTag::m_sLanguageString` fields.** `Theme` has an
unused, otherwise-dead forward declaration in `ThemeManager.h`
(`struct Theme;`, never actually referenced there) — its real
definition and every usage live in the `.cpp`.
`CompareLanguageTag` is fully file-local (a `std::partition`
predicate functor). Two `const RString&` hard boundaries for
`sThemeName` — `GetLanguagesForTheme`/`GetPathInfoToRaw`
(`ThemeManager.h:121,125`) — fixed with `RString(...)` wraps at the 2
call sites. `m_sLanguageString.MakeLower()` (an RString-only facade
method) replaced with the guarded free-function form
(`if(!s.empty()) MakeLower(&s[0], s.size());`) established in pilot
#1 — the very first pilot of this whole effort, confirming that
lesson still applies unchanged 27 pilots later. `.find(...)` on the
migrated field needed no change (inherited `std::basic_string` method,
not a facade override). Verified: `sm_tests` 5981/230 unchanged,
`ctest` 100%, Release `StepMania-R.exe` clean rebuild, `--SelfTest`
exit 0.

**Pilots #28-29 (2026-09-13, found via a second scouting fork after
`actor_template_t` was deliberately deferred as too large/unverifiable
for this session): `GameLoop.cpp`'s `g_NewTheme`/`g_NewGame` file-scope
statics, and `Style::ColToButtonName(int)`'s return type.**
`g_NewTheme`/`g_NewGame` are `static` (internal linkage, genuinely
file-local). 5 hard-boundary fixes, all local to `GameLoop.cpp`:
`THEME->SwitchThemeAndLanguage`/`IsThemeSelectable` (both
`const RString&`) and `PREFSMAN->m_sTheme.Set` (`Preference<RString>::
Set(const T&)`) each needed an `RString(...)` wrap at 2-3 call sites;
`GAMEMAN->StringToGame` (by-value) and `PREFSMAN->m_sTheme`'s own
implicit `operator const RString()` conversion needed no changes
(both already-established safe directions).
`Style::ColToButtonName` required touching 3 files: its own
`Style.h`/`.cpp` (return type change, plus `Style.cpp`'s
`lua_pushstring(L, p->ColToButtonName(iCol))` — needed `.c_str()`
added, since `lua_pushstring` wants `const char*` and `RString`
(unlike `std::string`) has an implicit `operator const CT*()` that was
silently covering this before); and 2 external files
(`GhostArrowRow.cpp`, `NoteDisplay.cpp`) that each bound the call's
result into `const RString &sButton`. Initially planned to retype
those to `const std::string&`, but `NoteDisplay.cpp`'s `sButton` alone
feeds ~11 more `.Load(sButton, ...)` calls all taking `const RString&`
— retyping would have cascaded into 11 more wraps in one function.
**Simpler fix: drop the reference and copy by value instead**
(`RString sButton = ...ColToButtonName(...)`) — `sButton` stays a
genuine `RString` throughout, so every downstream call keeps working
completely unchanged. Same fix applied to `GhostArrowRow.cpp` for
consistency, even though its own downstream use count was only 1.
Verified: `sm_tests` 5981/230 unchanged, `ctest` 100%, Release
`StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.

**CI-caught fix (2026-09-13): pilots #28-29's macOS jobs failed —
`GameLoop.cpp:183`, `Preference<RString>` → `std::string` assignment
compiles on MSVC but not Clang.** `g_NewTheme = PREFSMAN->m_sTheme;`
relies on `Preference<T>::operator const T()` (one user-defined
conversion) producing an `RString` prvalue, then binding that into
`std::string::operator=`. MSVC's STL accepts the derived-to-base
slice as part of the same conversion sequence; libc++/Clang's
templated `basic_string::operator=` does not, and rejects it as "no
viable conversion from `Preference<RString>` to `string`" — a genuine
cross-compiler difference **local Windows-only verification cannot
catch**, since AGENTS.md §4's gate is a Windows Release build, not a
multi-platform one. Fixed by using `Preference<T>::Get()` (returns
`const T&` directly, no implicit-conversion-operator hop) instead of
relying on the implicit conversion:
`g_NewTheme = PREFSMAN->m_sTheme.Get();`. Swept the rest of the
session's pilots for the same `PREFSMAN->m_s*` direct-assignment
shape (`grep -rn "= *PREFSMAN->m_s[A-Za-z]*;" src/*.cpp`) — the other
3 hits all target still-`RString` variables (same-type reference
binding after the one conversion, not a base-slice, so not affected).
**New standing lesson: prefer `Preference<T>::Get()` over the bare
implicit conversion whenever assigning a `Preference<RString>` into a
`std::string`-typed target** — it sidesteps this exact MSVC/Clang
divergence entirely. Verified locally (Windows): `sm_tests` 5981/230
unchanged, `ctest` 100%, Release rebuild, `--SelfTest` exit 0; pushed
and awaiting the macOS CI jobs specifically to confirm the fix (local
Windows verification cannot reproduce the original failure).

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
- ~~`modernize-use-equals-default` (38) and `readability-redundant-member-init`
  (28)~~ — **DONE 2026-09-14.** Was deferred because `--fix` output is
  too dirty to land without a coupled `clang-format` run (ADR 0002
  says that must be its own change). Asked the maintainer directly;
  answer was to do the full repo-wide `clang-format` for real, so:
  1. **Tooling gap found and closed:** neither `clang-format` nor
     `clang-tidy` were installed on this session's machine, and the
     official LLVM MSI installer needs admin rights (not available
     here — tried, got Windows error 1303 "insufficient privileges"
     even after retargeting the install dir, since the MSI always
     touches `Program Files` for some component regardless). Unblocked
     by downloading LLVM's **portable** Windows release
     (`clang+llvm-*-x86_64-pc-windows-msvc.tar.xz` from
     `github.com/llvm/llvm-project` releases) and extracting *only*
     `clang-format.exe`/`clang-tidy.exe` from it (no installer, no
     admin) into `~/bin` (already on `PATH`); also needed a portable
     Ninja binary (`ninja-build/ninja` releases) to generate
     `compile_commands.json` for clang-tidy, since the default VS-
     generator CMake build can't emit one — configured a `build-tidy/`
     Ninja tree per `baseline.md`'s existing clang-tidy recipe.
  2. **A real correctness bug caught before the repo-wide format
     landed, not just a formatting risk:** `.clang-format`'s
     `InsertBraces: true` is unsafe on this codebase. clang-format
     doesn't expand macros, so it can't see that
     `FOREACH_ENUM`/`FOREACH_CONST_Attr`/etc (used in ~98 files) expand
     to a brace-less `for(...)` header controlling the next bare
     statement — a real, common idiom here
     (`if (cond) FOREACH_ENUM(Type, x) statement;`). With
     `InsertBraces` on, clang-format mis-attributed the closing brace
     to the enclosing `if` instead of the `for` it couldn't see,
     silently moving the real loop body outside *both* the `if` and
     the `for` — confirmed via compile errors in `HighScore.cpp`/
     `NoteDataUtil.cpp`/`Profile.cpp` on the first attempt. Reverted
     that attempt in full, disabled `InsertBraces` in `.clang-format`
     as its own commit (`a38d1d225b`, reasoning recorded in the file
     itself), and re-ran clean — verified this time by programmatically
     checking all 33 real instances of the dangerous
     `if`/`FOREACH-or-for`/statement (brace-less, 3-line) shape in the
     tree, not just the 3 that happened to produce compile errors.
  3. **Repo-wide `clang-format` landed** (`a7f575fd83`): 993 in-scope
     `src/` files (excludes `Texture Font Generator/`, already excluded
     from tidy sweeps by convention, and `verstub.in.cpp`, a CMake
     `configure_file` template whose `@VAR@` tokens clang-format
     doesn't understand and would corrupt — caught this too, on the
     first attempt, before it reached a commit). ~231k total line diff.
     Own dedicated commit per ADR 0002 / the clang-tidy-subsystem-pass
     playbook's explicit warning against bundling a reformat into a
     tidy/logic PR.
  4. **The actual `--fix` applied** (`f2530e8bc8`): 63 sites across 43
     files (41 equals-default + 22 redundant-member-init), applied
     per-file sequentially with `--fix --format-style=none`, every
     hunk hand-reviewed, then `clang-format` run on just those 43
     touched files afterward to clean up the mechanical fix's
     raggedness (double-spaces where a removed init used to sit).
  Full gate green at every step (Release + Debug builds, ctest 100%,
  `sm_tests` 5981/230 unchanged, `--SelfTest`); `[corpus]`/`[crs]`/
  `[bms]` re-verified unchanged given 5 §5-adjacent files were touched
  by the final fix commit. `modernize-use-bool-literals` — DONE
  2026-09-09 (`3be07f669d`, closeout `3479323da9`), the diff was clean
  enough to land on its own, before any of the above.
- ~~`bugprone-integer-division` (14)~~ — **10 of 14 confirmed
  false-positive/intentional (2026-09-08 verdict, unchanged); the 4
  genuine ≤0.5px-imprecision sites fixed 2026-09-14** after asking the
  maintainer directly (`Font.cpp` FontPage::Load baseline/top calc,
  `SnapDisplay.cpp` arrow-indicator X position, `ScreenSelectCharacter.cpp`
  character-icon carousel Y position — the last one triggers on every
  render since `MAX_CHAR_ICONS_TO_SHOW` is literally `11`, odd;
  `NoteField.cpp`'s `ARROW_SIZE/2` found afterward once `clang-tidy`
  became available, currently exact since `ARROW_SIZE=64` but fixed for
  pattern-consistency). Commits `87db244dd0` (first 3) and
  `b14a82793a` (NoteField.cpp, completes 4/4). No automated test can
  catch a sub-pixel rendering shift, so this relied on the maintainer's
  explicit go-ahead rather than test coverage.
- 4 unfixable `bugprone-macro-parentheses` sites (`StatsManager` ×2
  `::`-scoped, `OptionRowHandler` MAKE(type), `Profile` LOAD_NODE(X)).
**This closes out every item-12 bucket that wasn't blocked by a
genuine external constraint.** What's left (§5-protected files needing
a regression corpus first, vendored `ixwebsocket`, the 4 unfixable
macro-parentheses sites) is blocked for real reasons, not effort.
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

### 15. `#if 0` dead blocks — three batches DONE, 8 remain (all correctly kept)
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
**Third pass, `Player.cpp`/`ScreenEdit.cpp` — DONE 2026-09-14
(`edc73a1c0a`).** Re-derived the actual current site count first (a
fresh full-tree grep found exactly 10 `#if 0` sites, matching the
first-pass total minus nothing lost): `NoteData.cpp`,
`RageDisplay_GLES2.cpp`, `RageFileManager_ReadAhead.cpp`,
`RageSoundReader_MP3.cpp`, `RageThreads.cpp`,
`RageUtil_CachedObject.cpp`, `RandomSample.cpp`,
`ScoreKeeperNormal.cpp` — already correctly identified above as
kept-for-reference or active `#else`/`#elif` selectors — plus
`Player.cpp`/`ScreenEdit.cpp`, the two genuinely removable ones.
**Key realization that unblocks this kind of site going forward: a
plain `#if 0 ... #endif` with no `#else` branch is never compiled
regardless of what its disabled logic does** — so deleting the dead
text cannot change runtime behavior by construction, once you've
confirmed (by reading to the literal matching `#endif`, this doc's own
long-standing warning) that it isn't secretly one arm of an active
selector and has no nested directives. That reduces "reason through
fragile gameplay/editor logic" to a much smaller, purely mechanical
check: locate the true boundary, confirm no `#else`, confirm no nested
`#if`/`#ifdef` inside. Both `Player.cpp` (a `PC_CPU`/`PC_AUTOPLAY`
tap-scoring cutoff, the author's own comment already called it "doesn't
make sense") and `ScreenEdit.cpp` (a `TransitionEditState` record-menu
shortcut superseded by the live `m_bReturnToRecordMenuAfterPlay`
handling elsewhere in the same function) passed that check cleanly.
Full gate green (Release + Debug builds, ctest 100%, sm_tests 5981/230
unchanged, `--SelfTest`).
**Remaining (8 sites, all previously confirmed as either active
`#if 0`/`#else` selectors or deliberate kept-for-reference blocks — see
the categorization above, none need further action unless one turns
out to be miscategorized on a future re-read):** `NoteData.cpp`,
`RageDisplay_GLES2.cpp`, `RageFileManager_ReadAhead.cpp`,
`RageSoundReader_MP3.cpp`, `RageThreads.cpp`,
`RageUtil_CachedObject.cpp`, `RandomSample.cpp`,
`ScoreKeeperNormal.cpp`. Out of scope: `archutils/Unix/*` and
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

### 18. Logging overhaul — ALL 4 PHASES DONE (2026-09-12, batch 22; see closure note after the batch history below)
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
  **Ph4 batch 13 (2026-09-12).** `RageDisplay.cpp` (5 real sites,
  `Log::General` — same reasoning as `RageDisplay_D3D.cpp`: it's the
  backend-agnostic base class, not GL-specific, so `Log::Gl` would be
  misleading; screenshot save/open failures upgraded `Trace`→`ERROR`,
  the `TryVideoMode`-fallback-ladder trace and FPS-stats dump kept
  `Trace` as routine), `Font.cpp` (3 real sites, `Log::Font` — a
  perfect fit; two "invalid codepoint value" font-definition warnings
  kept `WARN`, a "font page has no characters" routine note kept
  `Trace`), `ScreenEdit.cpp` (5 real sites, `Log::Screen` — "Save
  failed. Changes uncommitted from memory." upgraded `Trace`→`ERROR`,
  a genuine editor-save failure with real data-loss risk for the
  person editing a chart; the two save-*success* notices and the
  dtor/playback-start traces stayed `Trace`). No parsing/behavior
  logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~118 files remain on the corrected list.
  **Ph4 batch 14 (2026-09-12).** `RageUtil_BackgroundLoader.cpp` (7
  sites, `Log::File`, all `"XXX:"`-prefixed developer debug traces,
  already-correct routine `Trace`, just categorized), `MusicWheel.cpp`
  (6 real sites, `Log::Actor` — it's a `WheelBase`-derived UI widget,
  not a `Screen` itself, so `Actor` fits better than `Screen`; all
  already-correct routine `Trace`), `ImageCache.cpp` (3 real sites,
  `Log::Cache` — a perfect fit; "Converted X at runtime" and "image
  cache wasn't loaded" both kept `WARN`, matching the file's own
  "Warn and continue" comment), `Bookkeeper.cpp` (7 sites, `Log::File`
  — bookkeeping.xml read/write; two real XML-parse failures
  ("unexpected node", "Data node missing") upgraded `Warn`→`ERROR`,
  plus the never-invoked `WARN_AND_RETURN` macro upgraded the same way
  for consistency (dead in practice — no call site anywhere in the
  file — but live, uncommented code, migrated on the same terms as
  everything else); the per-entry "incomplete date field" and
  "Hour >= HOURS_IN_DAY" warnings kept `WARN` since they skip just
  that one malformed record and continue, not a whole-file failure;
  the write-failure ("Couldn't open file for writing") upgraded to
  `ERROR`). No parsing/behavior logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~114 files remain on the corrected list.
  **Ph4 batch 15 (2026-09-12).** `XmlToLua.cpp` (6 sites, `Log::File`
  — this is the XML-actor-def-to-Lua conversion utility; the "error
  loading xml" and "could not open output file" sites in the core
  `convert_xml_file` entry point upgraded `Trace`→`ERROR`, the
  optional sprite/model sub-file "failed to read" sites kept `Trace`
  since those files are legitimately optional per-actor), `SongManager.cpp`
  (6 sites, `Log::Song`, all already-correct routine `Trace`),
  `RageTextureManager.cpp` (6 sites, `Log::Cache` — the
  `"TEXTUREMAN LEAK"` shutdown-time refcount check upgraded
  `Trace`→`WARN`, a genuine resource-leak indicator; the rest are
  routine texture-inventory `Trace` dumps), `NoteField.cpp` (2 real
  sites, `Log::Actor`, both already-correct routine `Trace`),
  `GameState.cpp` (4 sites, `Log::General` — no dedicated category
  fits the central game-state singleton; the "`BeginStage` called
  twice" invariant-violation warning kept `WARN`, the blacklisted-name
  match upgraded `Trace`→`WARN` for the same "notable, audit-worthy
  guard event" reasoning as other content/security guards in this
  sweep, the two gameplay-flow traces stayed `Trace`). No parsing/
  behavior logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~109 files remain on the corrected list.
  **Ph4 batch 16 (2026-09-12).** Two files turned out to have zero
  real sites once checked carefully: `ScreenUnlockStatus.cpp` (all 5
  raw hits pre-existing `//`-comments) and `NoteDataWithScoring.cpp`
  (2 of its 5 raw hits sit inside a `/* ... */` block, the other 3
  are `//`-commented) — both skipped, nothing to migrate.
  `ScreenOptions.cpp` (3 sites, `Log::Screen`, all already-correct),
  `RageThreads.cpp` (5 sites, `Log::General` — no threading-specific
  category exists; the mutex lock-order-inconsistency warning
  upgraded `Warn`→`ERROR`, a genuine deadlock-risk bug indicator, not
  routine), `RageSoundReader_Merge.cpp` (2 sites, `Log::Sound` — a
  discarded-mismatched-channel-count warning kept `WARN`, a dev-debug
  "hurk" sync-correction trace kept `Trace` since channel-drift
  correction is routine during normal playback), `ProfileManager.cpp`
  (5 sites, `Log::Profile` — "profile corrupt, no LastGood backup
  either" upgraded `Trace`→`WARN`; "DeleteRecursive failed" upgraded
  `Warn`→`ERROR`; "ProfileID doesn't exist" kept `WARN`, a caller-side
  misuse, not a system failure), `LuaManager.cpp` (4 sites, `Log::Lua`
  — `ReportScriptError`'s and the Lua-compile-failure's `Warn`s both
  upgraded to `ERROR` since by definition every call through those
  paths is a real script error; the Lua-exposed global `Trace()`/
  `Warn()` functions kept at their respective levels since the level
  is the *theme author's* choice, not something to second-guess),
  `GameSoundManager.cpp` (4 sites, `Log::Sound`, all already-correct
  — including an opt-in (`PREFSMAN->m_bLogSkips`) song-position-skip
  diagnostic, routine when the debug pref is on). No parsing/behavior
  logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~101 files remain on the corrected list.
  **Ph4 batch 17 (2026-09-12).** `RageUtil_FileDB.cpp` (3 sites,
  `Log::File` — two documented-precondition violations
  ("m_Mutex was locked" when the API contract explicitly says it must
  not be) upgraded `Warn`→`ERROR`; "Slow cache due to" kept `WARN` as
  a routine perf diagnostic), `RageSoundReader_Chain.cpp` (3 real
  sites — a 4th grep hit sits inside a `/* ... */` block, caught by
  the same `awk` comment-state check used for `Player.cpp`;
  `Log::Sound`; "error opening sound" upgraded `Trace`→`ERROR`, the
  discarded-channel-mismatch and rate-desync warnings kept `WARN`,
  matching `RageSoundReader_Merge.cpp`'s precedent), `OptionRowHandler.cpp`
  (1 real site, `Log::Screen` — a theme/config issue: an option row
  with nothing selectable and no fallback, kept `WARN`), `Course.cpp`
  (2 real sites, `Log::Song` — an unrecognized sort-type reaching the
  `default:` case, whose own message already reads "invalid??",
  upgraded `Trace`→`WARN`; the "Total feet" stat dump kept `Trace`).
  No parsing/behavior logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~97 files remain on the corrected list.
  **Ph4 batch 18 (2026-09-12).** Skipped `ScreenNetEvaluation.cpp`
  (part of item 29's orphaned networking subsystem, not in the CMake
  build) and found a **third dead-code category**:
  `RageFileManager_ReadAhead.cpp`'s 3 grep hits all sit inside a
  permanent `#if 0` block (the real branch is
  `#if defined(HAVE_POSIX_FADVISE)` / `#else` / `#if 0` .../ `#else`
  (stub) / `#endif` / `#endif` — a preprocessor-disabled fallback path,
  never compiled on any platform, distinct from both `//` comments and
  `/* */` blocks already caught this sweep) — skipped, nothing real to
  migrate. Migrated: `ScreenMapControllers.cpp` (1, `Log::Input`),
  `ScreenHighScores.cpp` (2, `Log::Screen`), `ScreenEditMenu.cpp` (3,
  `Log::Screen` — "Delete failed; not deleting steps" upgraded
  `Trace`→`WARN`, a real user-visible action failure), `RageUtil_WorkerThread.cpp`
  (3, `Log::General`), `RageUtil_CharConversions.cpp` (3, `Log::File`
  — the two codepage/iconv "attempt failed" traces kept `Trace` since
  callers try multiple charsets speculatively; the partial-conversion
  warning kept `WARN`), `RageSoundReader_WAV.cpp` (3, `Log::Sound` —
  "predictor out of range" upgraded `Trace`→`WARN`, a genuine
  malformed-ADPCM-data anomaly; the duplicate-fmt-chunk and truncated-
  file warnings kept `WARN`), `RageFileDriverTimeout.cpp` (3,
  `Log::File`, all "X timed out" kept `Trace` — routine for a
  timeout-wrapper driver), `NetworkManager.cpp` (3, `Log::Net` —
  "reading CA bundle failed" upgraded `Warn`→`ERROR`, a real
  HTTPS-connectivity risk; the "blocked access" security-guard
  warnings kept `WARN`), `LyricsLoader.cpp` (3, `Log::Song` — an
  invalid `.lrc` color-value warning upgraded `Trace`→`WARN`). No
  parsing/behavior logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~87 files remain on the corrected list.
  **Ph4 batch 19 (2026-09-12).** `JsonUtil.cpp` (3 sites, `Log::File`
  — all three are genuine file-open/parse failures, upgraded
  `Warn`→`ERROR`), `InputMapper.cpp` (3 sites, `Log::Input` — two
  routine startup-diagnostic `Info`s, one routine "no mapping file
  yet" `Trace`, none changed level), `InputFilter.cpp` (3 sites,
  `Log::Input` — the zero-timestamp warning kept `WARN`; the
  out-of-range device/button index sites upgraded `Trace`→`WARN`,
  since a driver handing back an index outside the known
  device/button range is a real anomaly worth surfacing, not routine),
  `GameLoop.cpp` (3 sites, `Log::General`, all already-correct routine
  `Trace`), `ActorMultiTexture.cpp` (3 sites, `Log::Actor` — the two
  "can't add nil texture" / "index too high" theme-misuse guards kept
  `WARN`). No parsing/behavior logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~82 files remain on the corrected list.
  **Ph4 batch 20 (2026-09-12).** `WheelBase.cpp` (2, `Log::Actor`),
  `UnlockManager.cpp` (2, `Log::Song`), `StatsManager.cpp` (2,
  `Log::Profile`), `StageStats.cpp` (2, `Log::Profile`) — all four
  already-correct routine `Trace`. `ScreenWithMenuElements.cpp` (2,
  `Log::Screen` — both upgraded `Trace`→`WARN`: "Lua music script did
  not return a path to a sound" and "run script failed hardcore, lol"
  are both real theme-Lua-script failures, informal phrasing
  notwithstanding). `ScreenServiceAction.cpp` (2, `Log::Screen`),
  `ScreenSelectCharacter.cpp` (2, `Log::Screen`), `ScreenSelect.cpp`
  (1 real site, `Log::Screen`) — all already-correct routine `Trace`.
  No parsing/behavior logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~75 files remain on the corrected list — almost entirely 1- and
  2-site files from here, plus `Player.cpp`'s 47 raw hits (already
  fully migrated — 1 real site — but still shows in a raw count since
  46 are dead code).
  **Ph4 batch 21 (2026-09-12).** `Steps.cpp` (2, `Log::Steps`, both
  kept `WARN` — "unknown style" and "couldn't load NoteData" are real
  chart-load failures; section-5-protected file, re-verified via the
  transitive NotesLoader/GameManager characterization suite since no
  dedicated `test_Steps.cpp` exists — unchanged before/after).
  `ScreenEvaluation.cpp` (2, `Log::Screen`, `TRACE`), `ScreenDebugOverlay.cpp`
  (2, `Log::Screen` — "Game halted" kept `WARN` as a deliberate
  developer reminder per its inline comment, "DEBUG: %s" kept `TRACE`),
  `ScreenAttract.cpp` (1 real site, `Log::Screen`, `TRACE`).
  `RageSurface_Load_PNG.cpp` (2, `Log::File` — libpng error callback
  upgraded `Trace`→`ERROR`, libpng warning callback upgraded
  `Trace`→`WARN`). `RageBitmapTexture.cpp` (1 real site, `Log::File`,
  upgraded `Warn`→`ERROR` — surfaces a user-facing dialog).
  `RageInput.cpp` (2, `Log::Input` — ctor kept `TRACE`,
  "NO_INPUT_DEVICES_LOADED" kept `WARN`). `PrefsManager.cpp` (2,
  `Log::General` — "unknown preference" kept `WARN`, "restored
  preference to default" kept `TRACE`). `Preference.cpp` (2 identical
  sites, `Log::Lua`, `TRACE`). `PercentageDisplay.cpp` (2,
  `Log::Actor`, `TRACE`). `NoteSkinManager.cpp` (2, `Log::Actor`,
  `TRACE`). `InputQueue.cpp` (2, `Log::Input` — "ignoring empty code"
  kept `TRACE`, "unrecognized button" upgraded `Trace`→`WARN`, since an
  unrecognized button name in a mapped code is a real theme/config
  anomaly). `CsvFile.cpp` (2, `Log::File` — read-failure kept `TRACE`
  matching `IniFile.cpp` precedent, write-failure upgraded
  `Trace`→`ERROR` matching `IniFile.cpp` precedent). `BackgroundUtil.cpp`
  (1 real site, `Log::Song`, kept `WARN` — "Background missing"; the
  other raw hit at line 252 is pre-existing dead `//`-commented code,
  left untouched). `Background.cpp` checked and confirmed **0 real
  sites** (both raw hits are pre-existing dead `//`-commented code) —
  no edit needed, removed from the remaining-files list.
  `ActorUtil.cpp` (2, `Log::Actor`, both kept `WARN`).
  `RandomSample.cpp` (2, `Log::Sound`, both kept `TRACE` — confirmed
  real via `#if 0` structure check: the file's dead `#if 0`/`#else`/
  `#endif` block at lines 41-53 does not contain these two sites).
  No parsing/behavior logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  ~62 files remain on the corrected list.
  **Ph4 batch 22 (2026-09-12).** Cross-checked every file already
  marked "done" whose raw grep count was still non-zero (`Font.cpp`,
  `RageSound.cpp`, `NoteField.cpp`, `ImageCache.cpp`, `StepMania.cpp`,
  `RageSoundReader_Merge.cpp`, `RageDisplay.cpp`, `OptionRowHandler.cpp`,
  `Song.cpp`, `ScreenOptions.cpp`, `ScreenManager.cpp`,
  `ScreenGameplay.cpp`, `ScreenEdit.cpp`, `RageDisplay_D3D.cpp`,
  `GameState.cpp`, `Course.cpp`, `NotesLoaderSM.cpp`,
  `ScreenMapControllers.cpp`, `PlayerStageStats.cpp`) — all confirmed
  dead `//`/`/* */` remainders only, no missed real sites. **Found and
  fixed a genuine false-negative in the `awk` block-comment scanner**:
  `ScreenSoundReader_Chain.cpp`'s stray hit really is dead (confirmed by
  direct read, matches the earlier finding), but `ScreenPackages.cpp`'s
  real, live `LOG->Trace("end: ...")` site was wrongly flagged "inside a
  comment" by the awk script — its `GetDirListing("Packages/*.zip", ...)`
  glob-pattern string contains a bare `/*` that the naive line-based
  scanner can't tell apart from a real block-comment opener, so it
  treated everything from there to the next real `/* */` pair (hundreds
  of lines later) as commented out. **New standing lesson: the awk
  block-comment check is itself fooled by `/*`-shaped substrings inside
  string literals (glob patterns, URLs) — treat a "no real sites" awk
  verdict as a hint, not proof, and directly read the surrounding lines
  before trusting it, especially when the original grep hit had no `//`
  prefix.** Separately, `ScreenPackages.cpp` and `FileDownload.cpp`
  turned out to be **absent from every `CMakeData-*.cmake` list** —
  orphaned from the CMake build like item 29's networking cluster, but
  a distinct feature (the online package-downloader UI, not
  multiplayer/room sync). Left both unedited pending the same
  maintainer wire-vs-delete call as item 29; noted under item 29 below.
  Migrated 33 genuinely new real sites this batch: `AnnouncerManager.cpp`,
  `AttackDisplay.cpp`, `ComboGraph.cpp`, `GameCommand.cpp`,
  `EditMenu.cpp`, `Grade.cpp`, `FontManager.cpp`, `ModelManager.cpp`
  (→ `Log::Cache`, matches the `RageTextureManager`/`ImageCache`
  resource-cache-leak precedent), `MeterDisplay.cpp`,
  `MessageManager.cpp`, `PlayerState.cpp`, `PlayerOptions.cpp`,
  `RageException.cpp`, `RageFile.cpp`, `RageSoundManager.cpp`,
  `RageSoundPosMap.cpp`, `RageSoundReader.cpp`, `RageSurfaceUtils.cpp`,
  `RageSoundReader_FileReader.cpp`, `RageSurface_Load.cpp`,
  `RageSoundReader_Vorbisfile.cpp`, `RageSurface_Save_PNG.cpp`,
  `ScoreDisplayOni/LifeTime/Normal/Battle/Rave.cpp`, `Screen.cpp`,
  `ScreenOptionsManageEditSteps.cpp`, `ScreenOptionsMaster.cpp`,
  `ScreenOptionsMasterPrefs.cpp`, `ScreenStatsOverlay.cpp`,
  `ScreenTextEntry.cpp`, `ScreenTitleMenu.cpp`, `SongCacheIndex.cpp`,
  `SongUtil.cpp` — categorized `Log::General`/`Actor`/`Font`/`Sound`/
  `File`/`Screen`/`Song` per the established per-subsystem mapping, all
  already-correct routine `Trace`/`Warn` levels, no upgrades needed.
  `NoteDataUtil.cpp` (1 real site, `Log::Song`, `WARN`) and
  `NotesWriterDWI.cpp` (1 real site, `Log::Song`, `TRACE`) are
  section-5-protected — re-verified: `NoteDataUtil.cpp` against its
  dedicated `[NoteDataUtil]` characterization tag (53/12, unchanged
  before/after); `NotesWriterDWI.cpp`/`SongCacheIndex.cpp`/`SongUtil.cpp`
  have no dedicated tag, so the unchanged full-suite invariant is their
  safety check (same treatment as `Song.cpp` in batch 10). No
  parsing/behavior logic changed anywhere.
  Verified: `sm_tests` 5966/226 unchanged, `ctest` 100%, Release
  `StepMania-R.exe` clean rebuild, `--SelfTest` exit 0.
  Remaining tail is now down to a handful of files (mostly
  §5-protected loaders/writers not yet individually triaged, plus
  `Player.cpp`'s already-resolved 47 raw/1-real count) — regenerate via
  `grep -cE "LOG->Trace|LOG->Warn|LOG->Info" src/*.cpp | sort -rn` to
  confirm the true remaining count before declaring phase 4 complete.

  **CLOSURE (2026-09-12, same day as batch 22): phase 4 is now
  genuinely complete, this time verified via a true fresh full-tree
  re-sweep** — learning explicitly from the earlier premature-
  completion mistake (the correction note above this batch history).
  Every file the re-sweep's raw grep count still flags is now one of:
  (a) fully migrated, with only dead `//`/`/* */`-commented lines
  raising the count (confirmed file-by-file across batches 12-22); (b)
  a file the whole session has deliberately left untouched pending a
  maintainer decision — the item 29 SMOnline/multiplayer cluster
  (`NetworkSyncManager.cpp`, `ScreenSMOnlineLogin.cpp`,
  `ScreenNetEvaluation.cpp`, `RoomWheel.cpp`, etc.) and the newly-found
  item 29 package-downloader pair (`ScreenPackages.cpp`,
  `FileDownload.cpp`) — both orphaned from the CMake build; or (c)
  `RageFileManager_ReadAhead.cpp`'s permanently dead `#if 0` block
  (batch 18 finding). No non-blocking file remains unmigrated. 22
  batches total for phase 4 (batches 1-11 on the original top-30 list,
  batches 12-22 on the corrected full-tree list after the methodology
  fix) — see `DocsAgents/log.md` for the complete batch-by-batch
  history. **Item 18 is closed.**

### 20. Replace the archaic hard-coded game-type system — stage 1 DONE (ADR 0008)
**Stage 1 complete, 2026-09-16.** The hand-written `static const Game
g_Game_X = {…}` / `g_Style_*` / `g_AutoKeyMappings_*` struct literals
(~3600 lines) and the hand-maintained `g_Games[]` array are **gone**
from `src/GameManager.cpp`. All 12 games (dance, pump, techno, lights,
kb7, ez2, para, ds3ddx, beat, maniax, popn, kickbox) now live as an
ini-tree under `Games/<name>/` (`game.ini`, `autokeymap.ini`,
`styles/*.ini`; see `src/GameDataIO.h`/`.cpp`), generated field-for-field
from the old literals (proven via `tests/test_GameDataIO.cpp`'s
round-trip characterization test before the literals were deleted, and
CI-verified across Windows/macOS/Ubuntu both before and after deletion:
`dd56cf3d91`). `GameManager::LoadGames()` reads that tree into a
`GameDataStore` (`std::deque`-backed, stable addresses) and builds
`g_Games` as a plain `std::vector<const Game *>` at startup, right after
the `GAMEMAN` global itself is assigned — `LoadGameFromDisk` resolves
`#STEPSTYPE` strings through `GAMEMAN->StringToStepsType`, so it can't
run from inside `GameManager`'s own constructor. Adding a game is now:
drop a definition under `Games/<name>/` + a `NoteSkins/<name>/` folder —
no `.cpp` edit, no rebuild. (`NoteSkins/Para/`'s capitalization mismatch
was fixed separately, `ea82d3ab3d`.)

**What's left (bigger, its own ADR territory):** the compile-time
`StepsType` **enum** and its parallel `g_StepsTypeInfos[]` array in
`GameManager.cpp` are still hand-written C++ — stage 1 only replaced the
Game/Style/InputScheme/AutoMappings data, not StepsType itself. Turning
`StepsType` into a runtime id ripples through `NoteData`, `Style`,
`Steps`, `RadarValues`, score keepers, the editor, Lua bindings, `Profile`
serialization. **Hard constraint, unchanged:** the on-disk `#STEPSTYPE`
strings (`dance-single`, `pump-double`, `bm-single7`, `pnm-nine`, …) are a
stable contract with the ~20-year simfile library (`AGENTS.md` §5) —
every existing value must resolve identically, no mass cache
invalidation. Pick this up as its own ADR (a stage 2 of 0008, or a new
one) rather than folding it into stage 1's scope.

### 29. Networking/multiplayer (SMOnline) subsystem is orphaned from the CMake build — DELETED 2026-09-14 (`9887258c71`)
**Maintainer decided: delete, not revive** (asked directly, per this
item's own "needs a maintainer call" note below). Re-audited before
deleting and found the true component was bigger than originally
documented and was really **one connected orphan, not two separate
ones**: `ScreenNetEvaluation.cpp` (only the `.h` was previously listed)
was also orphaned, and `FileDownload.h`/`ScreenPackages.*` share the
SMOnline cluster's own dependency — `src/ezsockets.cpp`/`.h` (a
2004-era hand-rolled raw-socket wrapper, itself also absent from every
`CMakeData-*.cmake` list). Final deleted set: 24 files / 6731 lines —
`NetworkSyncManager.cpp/.h`, `ScreenSMOnlineLogin.cpp/.h`,
`ScreenNetSelectMusic.cpp/.h`, `ScreenNetworkOptions.cpp/.h`,
`ScreenNetRoom.cpp/.h`, `ScreenNetSelectBase.cpp/.h`,
`ScreenNetEvaluation.cpp/.h`, `RoomWheel.cpp/.h`,
`RoomInfoDisplay.cpp/.h`, `ScreenPackages.cpp/.h`,
`FileDownload.cpp/.h`, `ezsockets.cpp/.h`. Verified fully isolated
first — an exhaustive grep across every remaining `src/*.cpp`/`*.h`
confirmed zero files outside this set `#include` any of its headers or
reference `NSMAN`/`FileTransfer`/`EzSockets`. Reactivating this later
would have meant rewriting `ezsockets.cpp` against modern networking
APIs and re-validating an online protocol nobody has run against a
live server in years — not a simple CMake-wiring fix — which is why
delete was the practical call over reconnect. Full gate green (Release
+ Debug builds, ctest 100%, sm_tests 5981/230 unchanged, `--SelfTest`).

Original finding, kept for history: found 2026-09-11 while doing item
18 phase-4 batch 3: `NetworkSyncManager.cpp`
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

**Related finding, item 18 phase-4 batch 22 (2026-09-12):** a second,
distinct orphaned pair — `ScreenPackages.cpp` (the online
package-downloader UI) and `FileDownload.cpp` (its HTTP download
helper) — is likewise absent from every `CMakeData-*.cmake` list and
produces no object file. This is a different feature from the
SMOnline/multiplayer cluster above (no shared files), so it's a
separate wire-vs-delete question, but the same maintainer call applies.
Left unedited (not even the free category/level tagging this time,
since these two weren't already touched) pending that decision.

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
