# WP-04 progress (save model + upgrade pipeline), branch wp04

## Plan
1. Read spec (Game, GameParameters, Proto*, FlightState, KerbalRoster, ScenarioModule, ShipConstruct file path,
   SaveUpgradePipeline/*, Versioning, GamePersistence).
2. Port into include/ksp/save/ + src/save/ (and src/save/upgrade/).
3. Tests: round-trip gate over decompiled/KSP-dev/saves/**/*.sfs + *.craft + Ships/**/*.craft; synthetic upgrades;
   GameParameters defaults/presets.
4. STATUS.md WP-04 section, commit.

## Corpus facts
- saves: 105 .sfs (227 MB; versions 0.21.0 x2, 1.0.0 x9, 1.1.0 x3, 1.2.0 x15, 1.4.5 x1, 1.11.2 x14, 1.12.5 x61),
  118 .craft under saves, 91 stock .craft under Ships.

## Log
- Step 1 (reading) notes:
  * Game.cs: Game(ConfigNode root) parses GAME values in file order; Save writes fixed order: version, Title, Description,
    linkURL, linkCaption, Mode(name), Status(int), Seed, [ROCSeed if Serenity], [RemovedROCs node if Serenity+ROCManager],
    CometNames node, scene(int), editor(name), flag, launchID(uint), defaultVAB/SPHLaunchSite, PARAMETERS, SCENARIO*,
    FLIGHTSTATE, [MISSIONTOSTART], modded=(modded||GameDatabase.Modded), envInfo=GameDatabase.EnvironmentInfo (ENV!),
    versionFull, versionCreated, LoaderInfo (GameDatabase.SaveLoaderInfo), ROSTER, then additionalSystems
    (onGameStateSave listeners: MessageSystem MESSAGESYSTEM; mods e.g. VMSNode). Mode!=SANDBOX/SCIENCE resets launch sites.
  * ConfigNode.Save writes all values then all nodes -> value order independent of node order.
  * GameParameters: ParameterNode.Save = Mono reflection order: r/w properties (decl order) then fields (decl order).
    Custom params dict order = reverse of Title-sorted ParameterTypes (after first ctor): CommNetParams, AdvancedParams,
    MissionParamsGeneral, MissionParamsExtras, MissionParamsFacilities (Mission titles are raw "#autoLOC_" tags).
    Unknown custom param nodes (mods) are dropped with a warning.
  * FlightState: values version, UT, activeVessel, mapViewFiltering, commNetUIModeTracking/Flight (CommNetUI statics).
  * ProtoVessel/ProtoPartSnapshot: unknown values -> KSPParseable dict (vessel saves .value, part saves .Save() =
    "v, TYPE" -> unknown part values gain ", UNKNOWN" on first resave). PreFailState only if MakingHistory.
  * ProtoCrewMember: suit depends on installed DLC suits; FlightLog.Load REMOVES "flight" from the source node;
    APPLICANTS/RECRUIT and CREW nodes become KERBAL on save; SetExperienceTrait fills empty trait via name hash.
  * Upgrade scripts: 40 scripts (33 PartReplace, 3 PartOffset, 1 wheel overhaul, 2 v180 (need prefab ctrlSurfaceRange),
    1 ModuleReplace) + 11 framework files = 51. TypeDef order (shipped DLL via Mono.Cecil, harness/build/KspGolden):
    probeCoreSphere, v110_LandingGearOffset, v110_LandingLegLT1Offset, v110_LandingLegLT2Offset, v110_WheelModuleOverhaul,
    v1120_engineLargeSkipper, liquidEngine1_2, liquidEngine2_2, liquidEngine3, liquidEngineMini, microEngine, mk1pod,
    probeCoreOcto, probeCoreOcto2, radialEngineMini, RCSBlock, rocketNoseCone, ServiceBay_125, ServiceBay_250,
    Size3to2Adapter, solidBooster, solidBooster_sm, stackBiCoupler, stackTriCoupler, v1123_dockingNode, v170_fuelTank1_2,
    2_2, 3_2, 4_2, mk1_2, stackDecoupler, stackDecoupler1_2, stackDecouplerMini, stackDecouplerSize3, stackSeparator,
    stackSeparatorBig, stackSeparatorMini, toroidalFuelTank, v180_ModuleAeroSurface, v180_ModuleControlSurface.
    Pipeline sorts by (TargetVersion, EarliestCompatibleVersion) stable; iterations run scripts from the END.
    UpgradeScript.Upgrade appends "upgraded = <Name> // <Description>" to the ROOT node.
  * CustomParameterNode Mono GetTypes order (RID): MissionParamsExtras 2810, Facilities 2811, General 2812,
    CommNetParams 3331, AdvancedParams 3697 (nested types last).
  * Env-dependent save fields: envInfo, modded, LoaderInfo (GameDatabase), ROCSeed/RemovedROCs (Serenity), PreFailState
    (MakingHistory), suit (DLC suits), additionalSystems (MESSAGESYSTEM etc via onGameStateSave).
  * Craft: ShipConstruct.LoadShip/SaveShip are live-Part code; data side = header + per-part fields read by LoadShip
    + fields SaveShip writes from Part state (partName, modCost/modMass/modSize carried from file).

## Design (headers include/ksp/save/*, src/save/*)
- dotnet/version.hpp (System.Version), dotnet/enum_parse.hpp (full Enum.Parse)
- save/environment.hpp: GameDatabase info, ExpansionsLoader, HighLogic::CurrentGame, GameSettings extras, Versioning
  extras, FlightGlobals persistentId registry, random hooks.
- save/save_enums.hpp, game_parameters, crew (FlightLog, ProtoCrewMember, KerbalRoster, ExperienceSystemConfig),
  proto_vessel (snapshots), game (Game, FlightState, ProtoScenarioModule, ScenarioModule), game_persistence,
  ship_construct, upgrade_pipeline (+ generated stock scripts table).
- Step 2 (code) in progress. Written: include/ksp/dotnet/{version,enum_parse}.hpp + src/dotnet/version.cpp;
  include/ksp/save/{save_enums,save_environment,game_parameters,crew}.hpp; src/save/{save_util.hpp,game_parameters.cpp,
  crew.cpp}. Next: proto_vessel.hpp/.cpp, game.hpp/.cpp (Game, FlightState, ProtoScenarioModule, ScenarioModule),
  save_environment.cpp, upgrade pipeline, ship_construct, game_persistence, CMake, tests.
  NOTE: bash heredocs with apostrophes can fail -> use Write tool for C++ files; python heredoc edits work.
- (resumed after rate limit) Done since: Scientist modifiers fix in crew.cpp; proto_vessel.hpp/.cpp written; save_util.hpp
  got ArrayMinSize/TryParseEnumIntArray/Write*Array/AddNullableValue/FirstChildShared (defined in proto_vessel.cpp).
  Next: game.hpp/.cpp, save_environment.cpp, then build once before upgrade pipeline.
- WARNING: heredoc (bash and python-in-bash) turns backslash escapes into real chars. For code with backslashes use
  Write/Edit tools only.
- Written: game.hpp/.cpp, save_environment.cpp, tools/save_corpus (CMake added; --filter --verbose --out),
  tools/wp04/gen_upgrade_scripts.py -> src/save/upgrade_scripts_data.inc (40 scripts, KSP_UPGRADE_SCRIPT macro).
  Library builds. Findings: KSP MainMenu.Start calls GetDefaultParameters before any load -> corpus tool does the same
  (custom params order). envInfo raw line (leading/trailing spaces) taken from bytes.
- Upgrade pipeline written (upgrade_pipeline.hpp/.cpp + generated .inc); builds. sfs corpus: 105 files, R1 identical 1
  (physprofile, the only stock 1.12.5 save), others differ (modded content, old versions), R2 idempotent 105/105.
  Next: structural classifier in save_corpus (value/node added/dropped/changed per node type with documented rules),
  then ShipConstruct file form + craft corpus, unit tests, STATUS.
- MILESTONE (commit 1): save model + upgrade pipeline + ShipConstruct file form + corpus.saves ctest (structural
  classifier). Corpus: sfs 105 (61 KSP-1.12.5 files: physprofile byte-identical + 60 modded byte-identical after removing
  mod-owned elements KSP drops; 44 older files all differences documented), craft 209 (49 byte-identical, 27 more after
  dropping mod values, 4 srfN stale-mesh quirk, 129 older documented), R2 idempotent 314/314, 0 unexplained, 36 s.
  Next: unit tests (defaults/presets vs GameParameters.cs, synthetic upgrades, pipeline semantics, persistence naming),
  GamePersistence port, README/STATUS.
