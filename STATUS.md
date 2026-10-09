# kspcore status

Updated 2026-10-09. One command: `powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\run_tests.ps1`
(details in README.md). Local git repository in `core/` only, never pushed.

## Summary

| deliverable | state |
|---|---|
| 1. Scaffold | done: CMake 3.28+/C++20/MSVC 14.44 presets (Debug, RelWithDebInfo; Ninja), `run_tests.ps1`, vendored doctest 2.4.12 + nlohmann/json 3.12.0 (sha256 in `third_party/VERSIONS.md`), golden gate (`golden_runner` + `harness/compare/compare.py`) as ctest tests, Unreal link settings (/MD, IDL 0, /Z7, no /GL) with `build/RelWithDebInfo/kspcore.lib`, C ABI `include/kspcore/kspcore_c.h` |
| 2. WP-01 maths + Unity shims + FloatCurve | done, all 19 package files (except the native-only Unity functions listed below); every gate bit-exact |
| 3. WP-02 ConfigNode | done: Load/Parse/Save/ToString, value/node API, typed getters/setters, ParseExtensions, Guid, KSPParseable, type parsers; byte-exact on the whole corpus. UrlDir (GameData tree) deferred, see below |
| 4. WP-03 event bus, FSM, utilities | done (branch `wp03`): typed event bus + full GameEvents registry (469 events), KerbalFSM, KSPUtil engine-independent parts, Localizer + Lingoona fallback, UtilMath completed, .NET collection/enum semantics, Simplex; gated by unit tests incl. 6,437 values from KSP's IL (no golden dataset), see below |
| extra | in-game (Mono) runtime semantics from the CAPTURE-SPEC C2/C7 captures: System.Math, float32 evaluation rule, conversions, formatting, System.Random, UnityEngine.Random, KSPRandom |

## Test results (all 10 ctest tests pass in RelWithDebInfo and in Debug)

Every numeric column of every gate: **max ULP 0, max abs 0** (bit-exact, and the same decimal text). Tolerance files:
harness goldens use `harness/compare/tolerances.json` unchanged; in-game capture gates use `tests/tolerances_capture.json`
(0 ULP everywhere; no relaxation was needed).

| ctest | reference | rows compared | result |
|---|---|---|---|
| `golden.math_shim.f64` | harness golden `math_shim` table f64 (Vector3d, QuaternionD, UtilMath, CelestialFrame, Zup) | 532 x 10 cols | 0 ULP |
| `capture.math_shim.f32` | in-game `captures/out/math_shim_mono.json` table f32 (Mathf, Vector3, Quaternion under Mono) | 903 x 10 cols | 0 ULP |
| `capture.libm_mono` | in-game `captures/out/libm_mono.json`: f64 (25 System.Math fns x 3,093 inputs), f32 (15 x 3,093), f32ops (100,000), f32ops_wide (80,000), conv (390), iconv (72), tostring (4,200), parse (296), sysrand (288) | 312,066 | 0 ULP, all strings identical |
| `capture.unity_random` | in-game `captures/out/unity_random.json`: seeds, states, value, range_f, range_i, vec, edge, trace, trace_int, trace_vec, ksp_random | 8,676 | 0 ULP, all ints identical |
| `golden.confignode_roundtrip` | harness golden: corpus (47) + save_text (21); runner: 47/47 saved files byte-identical, 45/45 parse trees identical | 68 + 47 files + 45 trees | exact |
| `golden.curves_eval` | harness golden (== native Unity per C1): curves 366, keys 1,055, samples 28,484 | 29,905 | 0 ULP |
| `corpus.gamedata` | self-consistency: 368 GameData `.cfg` + 86 `.craft` (8.55 MB, 241,219 values, 77,117 nodes): all load, Save->Load->Save byte-identical, Parse == Load, 226 FloatCurve nodes load | 454 files | 0 failures |
| `capi.c`, `capi.ue_like` | C ABI from pure C and from C++ with UE flags (/GR- /EHs-c- _HAS_EXCEPTIONS=0 /fp:fast); FloatCurve values bit-exact vs golden through the ABI | - | pass |
| `unit` | 70 doctest cases, 12,156 assertions, incl. 6,437 WP-03 values from KSP's IL on .NET Framework 4.8 (`tests/data/wp03_reference.tsv`) and 29,028 .NET Framework 4.8 reference results (`tests/data/netfx_reference.tsv`: parse 6,041 doubles + 6,041 floats, G17/G/G9 formatting 7,118, Sin/Cos/Tan 3,005 each, Ceiling/Round 400 each) | - | pass |

Not gated any more by design: the harness's .NET-generated float32 math_shim rows. kspcore follows Mono (the game) and
differs from that .NET golden in 46 cells outside its tolerance (r0..r2): up to 6 ULP (Vector3.Project/ProjectOnPlane) and
`Mathf.Repeat(1e20f, 360f)` = 360 (Mono) vs 0 (.NET); kspcore equals the in-game capture in all 903 rows.

## WP-03 Event bus, FSM, misc core utilities (branch `wp03`)

State: done (40 of the 42 package files fully or partly ported; see "skipped"). **No `KSP_GOLDEN_DATASET`**: there is
no harness golden for WP-03 and none was added, so the package is **gated by unit tests**, including
`test_wp03_reference.cpp`, which replays 6,437 values computed by **KSP's own IL** (the harness's Assembly-CSharp build)
on .NET Framework 4.8 (`tools/wp03_reference`, data `tests/data/wp03_reference.tsv`): 0 mismatches. A mutation check
(20 corrupted cells) confirmed every row kind is really compared. Every ported function cites `File.cs:line`.

| area | ported (C++ location) | verification |
|---|---|---|
| C# delegates | `ksp::Delegate<R(Args...)>`: Bind/Lambda/Static, C# equality, Target, multicast `+ - += -=`, invocation (`core/delegate.hpp`); `Callback<...>` | unit |
| Event bus | BaseGameEvent (+FindEvent), GameEventsBase, EventVoid, EventData<1..4>, EventDataModifier, Event{Double,Float,Int}{Max,Min}, EventValueComparison, EventValueOperation, EventValueWrapper (`events/event_data.hpp`) | unit + KSP IL (call orders R1-R4, exception isolation, modifier chain, NaN/signed-zero folds, hidden eventName) |
| GameEvents registry | all **469 events** (383 EventData, 80 EventVoid, 6 EventDataModifier: 3 static + 3 in `Modifiers`), 7 nested classes, the 5 nested structs, FindEvent; generated by `tools/wp03/gen_game_events.py`; 109 forward-declared KSP/Unity types (`events/game_event_types.hpp`) | KSP IL: member path, event-name string and C# type of all 469 match (`GameEventsRegistry()`) |
| FSM | KerbalFSM, KFSMState, KFSMEvent, KFSMTimedEvent, KFSMUpdateMode/Callback/EventCondition/StateChange (`fsm/kerbal_fsm.hpp`); UnityEngine.Time shim (`core/unity_time.hpp`) | unit (Time.time is native: cannot run on .NET) |
| KSPUtil | DefaultDateTimeFormatter (all 16 IDateTimeFormatter members + 10 GetUT*/GetTimeSpan*), IDateFormatterExtensions, dateTimeFormatter get/set, CacheLocalStrings, LocalizeNumber, PrintSI, Print{Coordinates,Latitude,Longitude}, the 12 Print* forwarders, PrintModuleName, PrintLocalizedModuleName, PrintSpacedStringFromCamelcase, GetOrCreatePath, GenerateFilePathWithDate, MoveFile, StripFileExtension, SanitizeString/Filename/InstanceName, CheckDOSName, CalculateFolderSize, CheckVersion x2, ReplaceString, Heading/Bearing x4, PrintCollection, AppendValueToString, Matrix4x4 Add/Diag/ToDiagonalMatrix/OuterProduct (9), DeepCompare, GetPartName, GetPartInfo, GetAttachNodeInfo x4, GetLinkID, GenerateSuperSeed, WriteArray, ParseArray, WriteVector(Vector3d)/WriteQuaternion(QuaternionD), ParseVector3d x2, ParseQuaternionD x2, per-component ParseVector2/3/4/ParseQuaternion (`util/ksp_util.hpp`) | KSP IL: 3,776 date/time strings (Kerbin + Earth calendars, 30 UTs incl. negative/NaN/Inf/1e300, every option), cache sequences, shared date holder, SI, coordinates, LocalizeNumber, strings, versions, craft ids, seeds, vectors |
| Localizer | KSP.Localization.Localizer tag table, Format (0/params), GetStringByTag, TryGetStringByTag, TranslateBranch, AddTagValuesFromNode/ForLanguage, unescaping; Lingoona.Grammar managed fallback (`localization/localizer.hpp`) | KSP IL: Format/Lingoona rows; the full dictionary.cfg loads to the same 12,030 tags; the built-in English subset equals dictionary.cfg |
| UtilMath (WP-01 file, completed) | added Max/Min(params), MaxFrom/MinFrom (8 overloads), WrapAround(int) exceptions; fixed to System.Math semantics: ASinh..Coth (CRT Log/Cosh/Sinh), Flatten (Pow, Sign(NaN) throws), RoundToPlaces(double) (Pow), AngleBetween (Atan2), BSPSolver (Math.Min NaN/-0 rules); citations corrected to the real lines | KSP IL: 852 UtilMath values; golden.math_shim.f64 still 0 ULP |
| .NET semantics | `dotnet::ListSort` (ArraySortHelper introsort), `dotnet::Dictionary` (.NET enumeration order, free-slot reuse), `dotnet::EnumToString/GetName/GetNames/Parse` (+[Flags], duplicates), `Math::Min/Max`, UTF-16 helpers | KSP IL / .NET: sort permutations (n = 17..1000 with ties), dictionary orders, enum strings |
| Collections / extensions | ArrayExtensions.IndexOf, ListExtension (Shuffle, AddUnique(Range), MaxAt/MinAt, SetRange x4, ContainsId x3, OrderByAlphaNumeric x2), BiDictionaryOneToOne, DictionaryValueList, ListDictionary, EnumExtensions (Description, displayDescription, GetDescriptions, ToStringCached), StringExtensions.GetHashCode_Net35, StringBuilderCache.Format, MinMaxColor, IntersectInformation, DelegateExtensions/CallbackUtil.Clear | unit + KSP IL (hash, Format, DVL, OrderByAlphaNumeric, enums) |
| Simplex | Simplex (instance, octaves, seed), SimplexNoise (static) (`util/simplex.hpp`) | KSP IL: 88 noise values (default tables); seeding (UnityEngine.Random is native) by reasoning on the bit-exact UnityRandom |

Tests: `unit` now 70 doctest cases / 12,156 assertions (WP-03: test_events 17 cases, test_util 14, test_wp03_reference 1
with 6,437 compared values). All 10 ctest tests pass in RelWithDebInfo (13.6 s) and Debug (27.1 s).

**Mono-rule float32 paths, verified by reasoning only** (the .NET reference cannot judge them; candidates for a future
in-game capture): KSPUtil.HeadingRadians/HeadingDegrees/BearingRadians/BearingDegrees, UtilMath.WrapAround(float),
RoundToPlaces(float), BSPSolver(float), MaxFrom/MinFrom float overloads (no arithmetic), MinMaxColor.GetLerp
(Color.Lerp), CalculateFolderSize (float sum), EventFloatMax/Min (comparisons only), LocalizeNumber(float) text.

**Skipped, and where it goes**
- Unity-bound KSPUtil members -> shell / later packages: FindInPartModel, recurseModels, GetTransformPathToRoot,
  GetTransformIndexPathToRoot, FindTransformAtIndexPath, HasAncestorTransform, HasDescendantTransform,
  FindComponentsImplementing, FindTagsInChildren (Transform/GameObject: UE5 shell, tier C), RemoveNonHighlightableRenderers
  (rendering, WP-48), ClampRectToScreen (UI), GenerateTexture2DArray (rendering), GetPartByCraftID (needs Part: WP-13),
  ApplicationFileProtocol (platform), GetActivator/ObjectActivator (reflection; WP-06 generated factories),
  SystemDateTime.DateTimeNow (only used by GenerateFilePathWithDate, which reads the local clock directly),
  GetRelativePath (System.Uri escaping; only the craft-thumbnail UI uses it).
- CallbackUtil coroutines (DelayedCallback x12, WaitUntil, DoUntil, HoldUntil, LazyCallback): Unity coroutines ->
  the game-loop scheduler (tier B, with the scene/loop package). Only CallbackUtil.Clear is ported.
- SafeAbortBackgroundWorker: .NET BackgroundWorker + Thread.Abort -> a C++ job/stop_token design where it is used
  (tier B).
- LocalizeNumber with formats other than G/F/R ("N0", "0.0", "0.###E+0", ...): throws std::logic_error until ksp::dotnet
  ports Mono's NumberFormatter (custom/N/E/P formats; needed by PartModule GetInfo strings, WP-06+).
- Localizer language switching from GameDatabase / buildID64.txt, mission tags and the native Lingoona engine: WP-51.

**Surprises (a later implementer must know)**
1. **Fire order is REVERSE registration order** (not registration order): every event class walks its snapshot from
   the end. EventDataModifier's DefaultModifier is registered first and therefore runs last.
2. **Re-entrancy**: each event has ONE shared snapshot list. Listeners removed or added during Fire do not affect the
   current round, but a listener that fires the same event again refills the shared snapshot; the outer loop then
   continues on the new list (can repeat or skip listeners), and if the new list is shorter the outer index throws
   ArgumentOutOfRangeException out of Fire (confirmed on KSP's IL: R2-R4).
3. Remove takes the LAST matching registration; Add of a static-method delegate throws NullReferenceException
   (`evt.Target.GetType()`), so the `originator == null` cleanup branch in Fire is dead code.
4. EventValueComparison/Operation.Update have no try/catch (an exception escapes and leaves the nesting counter at 1);
   their debug logs print an empty name (`new string eventName` / `gameEventName` are never assigned; KSP IL shows null).
5. GameEvents has duplicate event names (the later declaration wins in FindEvent): onPartCouple/onPartDeCouple,
   onPartCoupleComplete/onPartDeCoupleComplete, OnFlightState x2, OnApplicantHired (OnCrewmemberHired/Sacked),
   OnEVAConstructionModePartAttached (Attached/Detached), "Active Node Changed" x2.
6. KerbalFSM.updateFSM keeps its loop index across a state change, so it continues on the NEW state's event list.
7. KSPUtil date formatting: PrintTimeLong has no space before units ("1Hour"); PrintTimeCompact never shows years;
   PrintTime(0, n, explicitPositive: true) is "+ "; a fresh formatter's PrintDate(t, false) for t in [0, 60) and
   PrintDateCompact/PrintTimeStampCompact for floor(t) == -1 return null (kspcore ""); a UT too large for (int)
   (e.g. 1e300) throws OverflowException from Math.Abs(int.MinValue).
8. UtilMath.BSPSolver does not take |f| of the starting value: a negative start never moves (KSP IL confirms).
9. ListExtension.MaxAt/MinAt always throw for non-empty lists (loop starts at list[Count]); IntersectInformation's
   constructor swaps the two distances; DelegateExtensions.Clear / CallbackUtil.Clear do nothing.
10. Simplex.seed rewrites a STATIC permutation table shared by every Simplex and reseeds UnityEngine.Random globally;
    fastfloor(0) = -1.
11. SanitizeInstanceName("mk1pod (Clone)") = "mk1pod " (the "(clone" match overrides the " (" match).

**Deviations (documented, deliberate)**: C# null strings returned by the formatter caches are ""; FindEvent finds
nested-class events without the C# lazy class initialisation; a destroyed C++ event unregisters its name; log texts use
`e.what()` and C++ type names where C# prints Exception.ToString() / Type names; char.IsUpper/ToUpper cover Latin-1,
Greek and Cyrillic only; OrderByAlphaNumeric uses `dotnet::CompareCulture` (exact for ASCII); `int.MinValue % -1`
throws OverflowException as on .NET (unverified on Mono); List.Sort's depth limit uses Count unless the caller passes
the List capacity.

**Shared files touched** (merge notes): `CMakeLists.txt` (KSP_PROJECT_ROOT searches upwards for harness/, so git
worktrees under core/.worktrees build), `include/ksp/config/config_node.hpp` (NullReferenceException moved to
`core/exceptions.hpp`), `include/ksp/dotnet/math.hpp` (Math.Min/Max), `include/ksp/math/util_math.hpp` +
`src/math/util_math.cpp` (completion and System.Math fixes above), `src/config/kspparseable.cpp` (ParseVector4 /
ParseQuaternion warning text with the actual separator), `src/CMakeLists.txt`, `tests/CMakeLists.txt`, README.md.

Commits on `wp03` (from main `c051578`, not merged): `05e84fc` delegates + event bus + GameEvents registry, `072e37e`
KerbalFSM, `ecd9075` KSPUtil/Localizer/UtilMath/.NET semantics/collections/Simplex, `b0f9ba3` KSP-IL reference run,
`2a2bbb7` KSPUtil double vector helpers + README, then the docs commit (STATUS.md, PROGRESS-wp03.md).

## Build and test times on this machine (laptop, MSVC 14.44, Ninja)

| | configure | build (from clean) | ctest (10 tests) |
|---|---|---|---|
| RelWithDebInfo | 2.0 s | 12.6 s | 12.9 s (capture.libm_mono 8.8 s, corpus 2.4 s) |
| Debug | 1.9 s | 6.4 s | 33.2 s (capture.libm_mono 20.1 s, corpus 9.8 s) |

Incremental rebuild after a one-file change: ~1-2 s. `build/RelWithDebInfo/kspcore.lib` is 9.6 MB (/Z7 debug info
embedded).

## Commits (core/, branch main)

| hash | content |
|---|---|
| `d69e30f` | scaffold: CMake/presets, golden gate, .NET runtime layer, vendored test/JSON libs |
| `cd4366d` | WP-01 maths: Mathf/Vector/Quaternion, Vector3d/QuaternionD/UtilMath/CelestialFrame |
| `83d761c` | WP-02 ConfigNode |
| `c75c6c2` | WP-01 curves (AnimationCurve, FloatCurve) + GameData corpus test |
| `c4cf5a8` | WP-01 in-game Mono semantics (C2/C7), UnityRandom, KSPRandom, System.Random, capture gates |
| `bf43535` | WP-01 remaining files: DoubleCurve, Vector3/4Curve, Matrix4x4D, Mathfx, extensions, PIDclamp |
| `8992c77` | scaffold: Unreal link settings + C ABI |
| `854766d` | WP-02 KSPParseable, KSPUtil vector helpers, type parsers |
| `d229056` | docs: STATUS.md, PROGRESS.md |

The first four commits were each built and tested in a clean worktree (each passes the gates it contains).

## Incomplete, and why

- **WP-02 UrlDir/UrlFile/UrlConfig (GameData tree, UrlDir.cs, 927 lines)**: not ported. It crawls GameData with
  selective-expansion / limited-parts options, file-type tables and Localizer hooks that belong with GameDatabase
  (WP-05), and there is no golden for it yet; the `part_database` golden of WP-05 is the natural gate. ConfigNode,
  which it uses, is complete.
- **ConfigNode reflection mapping** (CreateConfigFromObject, LoadObjectFromConfig, [Persistent], links): WP-06 by plan.
  `ConfigNode.Load(path, bypassLocalization: false)` would call `Localizer.TranslateBranch` (not ported; `Load(path)`
  bypasses it, as in KSP). `TryGetEnum` takes an explicit member table (no reflection in C++).
- **Native Unity maths** (Quaternion.Euler/AngleAxis/LookRotation/FromToRotation/Slerp/RotateTowards,
  Vector3.Slerp/RotateTowards/OrthoNormalize, PerlinNoise): not ported; C3 captured them raw
  (`captures/out/math_shim_native.json`), which is the golden for the next step. `Quaternion.Inverse` (a plain
  conjugate, confirmed by C3) is provided. QuaternionD's extern members are dead in game (MissingMethodException) and
  are deliberately absent.
- **AnimationCurve weighted (Bezier) segments and Loop/PingPong wrap modes**: throw `std::logic_error`, as in the
  harness shim; KSP never produces them (C1: weightedMode 0 and ClampForever everywhere).
- **UnityRandom KSP consumers** (`ksp_class_radius`, `ksp_asteroid` tables of the C2 capture) need DiscoveryInfo / Orbit
  (later packages); the tables are excluded from the gate.

## Known approximations and risks

- **VS2013 CRT dependency**: Mono and .NET Framework 4.8 use the Visual Studio 2013 CRT libm. kspcore loads it at run
  time from `msvcr120_clr0400.dll` (installed with .NET Framework 4.x on every Windows 10/11) or `msvcr120.dll`
  (`dotnet::LibmBackend()` reports which; it is part of `kspcore_build_info()`). Without it (non-Windows), kspcore
  falls back to `<cmath>`, which differs in the last bit for cosh (130/3,093 inputs), sinh (40), pow (17), tanh (8),
  asin (3), atan (2), exp (1), atan2 (1). A portable bit-exact port of those VS2013 routines is future work.
- **Mono number formatting**: Mono builds 17 digits from a 64-bit approximation (MantissaBitsTable) before rounding;
  kspcore uses the correctly rounded 17 digits, then Mono's half-up rounding. This matched all 4,200 captured strings
  (including the known Mono/.NET G15 difference), but the approximation itself is not ported.
- **Mono Pow special cases**: only `Pow(+-0, negative) = +Infinity` is evidenced by the capture and implemented; other
  Mono wrapper rules (sysmath.c) are not verified.
- **Mono float32 rule at JIT spills**: Mono can also round where its JIT spills a stack temporary; following the C#
  source's own locals reproduced every probe and all 903 math_shim rows, but arbitrary code may hit IL shapes not yet
  seen (C7 `locals` probe).
- **.NET string semantics on UTF-8**: culture-sensitive `IndexOf`/`StartsWith` are ordinal here (identical for the ASCII
  syntax of cfg files); `ValueList.SortByName` uses an en-US-like comparison with a stable sort (List.Sort is unstable);
  invalid UTF-8 bytes become U+FFFD one byte at a time.
- **Thread safety**: `SetRuntime`, the log sink and `UnityRandom::Global()` are process-wide; AnimationCurve evaluation
  is stateless except for curves with duplicate key times (faithful per-curve cache, not thread-safe).

## Surprises a later implementer must know

1. **The harness's float32 goldens are .NET, not the game.** Mono evaluates C# float expressions in double and rounds
   only at stores/arguments/returns/casts. Port managed float code with `f32store(<double expression with fx()>)` at
   exactly the C# store points (`include/ksp/dotnet/mono_float.hpp`); never for native Unity code. Gate float32 paths
   against `harness/captures/out/*_mono.json` where they exist.
2. **Math.Sin/Cos are x87 FSIN/FCOS** on both runtimes (66-bit pi reduction: `Cos(Math.PI/2) = 6.1230317691118863E-17`,
   `Sin(x) = x` for |x| >= 2^63). kspcore emulates them exactly (192-bit reduction, double-double kernel); do not call
   `std::sin` in ported code, call `dotnet::Math::Sin`. Tan differs between the runtimes (Mono = CRT tan).
3. **double.Parse is not correctly rounded** in KSP's runtime: it is the CLR/Mono NumberToDouble (64-bit mantissa,
   power-of-ten tables read from mono-2.0-bdwgc.dll). Use `dotnet::ParseDouble/TryParseDouble`, never `strtod`.
   "-0" parses to +0; "1,000" parses as 1000 (AllowThousands); float overflow is a parse failure.
4. **double.ToString() on Mono rounds twice** (17 digits, then half-up to 15): `5.997450259893145` prints
   `5.99745025989315`. G17/G9/R match .NET on everything seen so far.
5. **FloatCurve/AnimationCurve is native code**: per-operation float32, bit-exact against native Unity (C1). Its
   evaluation is stateless in kspcore (identical to Unity's cached evaluation for strictly increasing key times).
6. **ConfigNode**: `//` starts a comment anywhere, a root-level `}` ends the file, ids survive ToString but not Save,
   `SetValue(..., createIfNotFound)` returns false when it creates and does not sanitize, `RemoveValue` removes the last
   match, `RemoveValues(string)` matches whole names, `AddValue` sanitizes `{}` (so a braced GUID cannot be stored),
   copying a tree with an anonymous child throws (NullReferenceException in KSP).
7. **Unreal**: link only through `kspcore_c.h`; never include `include/ksp/**` from UE code (inline float code would be
   recompiled with UE's /fp:fast and break the one-definition rule). The lib is built /MD, IDL 0, /Z7, no /GL.
