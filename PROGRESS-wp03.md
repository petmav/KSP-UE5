# WP-03 progress (Opus implementer, worktree core/.worktrees/wp03, branch wp03)

## Scope (work_packages.csv WP-03, 42 files, 7,095 lines) + brief extras
Event bus: BaseGameEvent, EventVoid, EventData(1..4), EventDataModifier, Event{Double,Float,Int}{Max,Min},
EventValueComparison, EventValueOperation, EventValueWrapper, GameEvents (+GameEventsBase), Callback, CallbackUtil,
DelegateExtensions. FSM: KerbalFSM, KFSM*. Utils: KSPUtil (engine-independent parts), IDateTimeFormatter,
IDateFormatterExtensions, ArrayExtensions, BiDictionaryOneToOne, DictionaryValueList, EnumExtensions, ListDictionary,
ListExtension, MinMaxColor, SafeAbortBackgroundWorker, Simplex, SimplexNoise, StringBuilderCache, StringExtensions,
IntersectInformation. Brief extra: finish UtilMath (WP-01 file; missing Max/Min/MaxFrom/MinFrom; check System.Math use).

## Status
- [ ] read sources
- [x] read sources (events, FSM, KSPUtil, UtilMath, collections, Simplex) -- baseline 10/10 ctest green in worktree
- [x] CMakeLists: KSP_PROJECT_ROOT now searches upwards for harness/compare/compare.py (worktree-safe)

## Key findings (C#)
- EventVoid/EventData*/EventDataModifier/Event*Max/Min: Fire iterates the snapshot in REVERSE registration order;
  per-listener try/catch (LogError + LogException) except EventValueComparison/Operation.Update (no catch).
  Shared per-event eventsClone + numEventsFiring: nested Fire of the same event re-clones the list the outer loop is
  indexing (outer continues on the new snapshot; may throw ArgumentOutOfRange outside try). Remove = last matching
  delegate. Add(static method) -> NRE (evt.Target.GetType()). originator==null branch dead (object ref compare).
- EventDataModifier ctor adds DefaultModifier (fires last). EventValueComparison has `new string eventName` never set
  (debug logs show ''), EventValueOperation logs `gameEventName` (never set).
- GameEvents: 470 `new` decls (378 EventData, 80 EventVoid, 3+3 modifiers in GameModifiers instance + 3 static
  EventDataModifier, 2 internal). Duplicate names (last wins in eventsByName): onPartCouple/onPartDeCouple,
  onPartCoupleComplete/onPartDeCoupleComplete, OnFlightState x2, OnApplicantHired x2, OnEVAConstructionModePartAttached x2,
  "Active Node Changed" x2.
- KerbalFSM.updateFSM keeps index i across a transition (continues on new state's event list); RunEvent with null
  GoToStateOnEvent -> NRE after lastEvent set. StartFSM calls OnEnter(null) before stamping time.
- KSPUtil DefaultDateTimeFormatter: static dateHolder shared; PrintDate cache returns null on first call with
  (includeTime=false, includeSeconds=false) when y/d/h/m all 0; PrintTimeStampCompact/PrintDateCompact cache hit on
  floor(time)==-1 with fresh formatter -> null. Caches ignore KERBIN_TIME.
- UtilMath WP-01 port: ASinh..Coth use std::log/cosh/sinh, Flatten std::pow, AngleBetween std::atan2, BSPSolver std::min
  -> must use dotnet::Math (CRT) and .NET Math.Min semantics. Missing Max/Min(params), MaxFrom/MinFrom (8 overloads).
- ListExtension.MaxAt/MinAt always index list[count] -> always throw for non-empty lists (sic).
- IntersectInformation ctor swaps distances (sic). DelegateExtensions.Clear / CallbackUtil.Clear are no-ops (sic).
- Simplex.seed mutates the STATIC p table shared by all instances and reseeds UnityEngine.Random; fastfloor(0) = -1.

## Step 1 DONE: event bus (commit below)
- include/ksp/core/{exceptions,delegate}.hpp (NullReferenceException moved out of config_node.hpp), src/core/delegate.cpp
- include/ksp/events/event_data.hpp, src/events/base_game_event.cpp
- GameEvents registry GENERATED: tools/wp03/gen_game_events.py (+cs_types.py type index) ->
  include/ksp/events/{game_events,game_event_types}.hpp, src/events/game_events.cpp. 469 events (383 EventData,
  80 EventVoid, 6 EventDataModifier) + Modifiers instance; 109 forward-declared types.
- tests/test_events.cpp: 13 cases / 96 assertions green.
## Next
- FSM (KerbalFSM + KFSM*, Time shim), util (Localizer shim, GameSettings shim, KSPUtil, collections, enums, Simplex,
  strings), UtilMath completion (+ dotnet Math.Min/Max), .NET reference tool (tools/wp03_reference, KSP IL via
  harness/build/KspGolden DLLs, read-only) -> tests/data/wp03_reference.tsv.

## Step 2 DONE: KerbalFSM (include/ksp/fsm/kerbal_fsm.hpp, src/fsm/kerbal_fsm.cpp, include/ksp/core/unity_time.hpp)
- 4 FSM test cases in tests/test_events.cpp (green). Committed.

## Step 3 DONE: utilities (all ctest 10/10 green; unit 69 cases)
- Localizer port (include/ksp/localization/localizer.hpp, src/localization/localizer.cpp) incl. Lingoona managed fallback.
- KSPUtil (include/ksp/util/ksp_util.hpp, src/util/ksp_util.cpp): DefaultDateTimeFormatter + statics; GameSettings
  KERBIN_TIME + Versioning shims live in ksp_util.hpp.
- dotnet: collections.hpp (ListSort = ArraySortHelper introsort, Dictionary with .NET order), enum.hpp, utf16.hpp,
  math.hpp Math.Min/Max; core/exceptions.hpp ArithmeticException.
- util: collections.hpp (ArrayExtensions, ListExtension, BiDictionary, DictionaryValueList, ListDictionary),
  enum_extensions.hpp, misc_util (StringExtensions, StringBuilderCache.Format, MinMaxColor, IntersectInformation,
  Clear no-ops), simplex (Simplex, SimplexNoise).
- UtilMath completed: Max/Min(params), MaxFrom/MinFrom x8, WrapAround(int) exceptions, dotnet::Math in
  ASinh..Coth/Flatten/RoundToPlaces/AngleBetween, .NET Math.Min in BSPSolver; citations fixed to the real line numbers.
- Found: BSPSolver's initial value is not abs'd -> a negative start never moves (sic).
## Next
- .NET reference tool (tools/wp03_reference): csc.exe (Framework64 v4.0.30319) + KSP IL from harness/build/KspGolden
  (read-only copy into build/wp03_reference) -> tests/data/wp03_reference.tsv + tests/test_wp03_reference.cpp.
- STATUS.md WP-03 section, README updates, final commit.

## Step 4 DONE: KSP-IL reference + docs
- tools/wp03_reference (csc.exe C# 5 + harness KspGolden DLLs copied to build/wp03_reference) -> tests/data/wp03_reference.tsv;
  tests/test_wp03_reference.cpp: 6,437 values, 0 mismatches (mutation check: 20/20 corrupted cells caught).
- GameEventsRegistry() listing (generated) checked against KSP IL: 469/469 member paths, names, types.
- Added KSPUtil double vector helpers; README + STATUS.md WP-03 section.
- Final: ctest 10/10 in RelWithDebInfo (12.4 s) and Debug (26.0 s); unit = 70 cases / 12,156 assertions.
## Open / follow-ups (for the orchestrator)
- Mono NumberFormatter port (custom/N/E/P formats) for LocalizeNumber callers.
- In-game capture for the Mono-rule float32 paths listed in STATUS.md.
- CallbackUtil coroutines / SafeAbortBackgroundWorker belong with the game-loop scheduler (tier B).
