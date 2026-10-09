# Feasibility Assessment: Reimplementing Kerbal Space Program on Unreal Engine 5

Date: 2026-10-08
Orchestrator: Claude Fable 5.1. Research: four parallel Haiku agents (legal/prior art, UE5 technical fitness, KSP architecture and scope, effort calibration). Proposed implementer: Claude Opus 5.5.

Evidence quality note: most research findings come from forum posts, Epic documentation, GitHub repos and news coverage rather than controlled benchmarks. Items marked UNVERIFIED were not confirmed against a primary source. The research agents flagged gaps in: UE5 empty-scene overhead vs Unity, Lumen cost, mod.io/AngelScript/Verse, Unity DOTS and Bevy comparisons, and current KSP1 sale status.

---

## 1. Verdict in one paragraph

A KSP-like on UE5 is technically achievable but the premise needs two corrections. First, this cannot be a port. KSP's source is proprietary and its publisher has litigated against a decompilation-based reimplementation, so this must be a clean-room rebuild from observed behaviour and public mechanics. Second, UE5 does not deliver the performance win. KSP's bottleneck is a single-threaded jointed rigid-body tree plus per-part main-thread updates and GC churn. UE5's headline features (Nanite, Lumen, Large World Coordinates, Mass Entity) do not touch that bottleneck, and UE5's Chaos physics is anecdotally worse than PhysX for long joint chains. The performance gain you want comes from architecture, not engine choice: double-precision orbital state outside the engine, a vessel physics model that is not N jointed bodies, data-oriented part simulation, and Kepler on-rails propagation. The recommended path is an engine-agnostic C++ simulation core with UE5 as a rendering and tooling shell, gated by a short spike phase that measures the two biggest unknowns before committing.

Overall feasibility: **Feasible as a multi-year project. UE5 is a defensible but not optimal shell. Full KSP1 parity is a 5 to 10 year volunteer-team-scale effort; agent-heavy development may compress it to 1 to 3 years, but no evidence exists for that compression on engine-heavy C++ work.**

---

## 2. Premise corrections

### 2.1 "Port" is not available; this is a clean-room reimplementation

- KSP1 is Unity 2019 / C# / PhysX 3.4. Source is closed. Assembly-CSharp.dll is trivially decompilable and the modding community has done so for years, but tolerance is not a license.
- Take-Two (owner of the KSP IP, acquired separately from Squad) issued DMCA takedowns and then sued the maintainers of re3/reVC, a GTA reimplementation that shipped no assets and required users to own the game. The sole offence was reverse-engineered code. That is the exact fact pattern a "decompile and translate" approach would create.
- Take-Two's EULA bars reverse engineering without written consent and assigns rights in "Custom Content" to Take-Two. Whether the current Take-Two EULA governs KSP1 purchases was UNVERIFIED.
- IP status in 2026: Private Division was sold to Haveli Investments in late 2024 with former Annapurna staff reportedly operating the portfolio. Whether the KSP IP itself transferred was UNVERIFIED. Intercept Games closed June 2024; KSP2 development ended.

Legal risk tiers:

| Approach | Risk | Notes |
|---|---|---|
| Clean-room from observed behaviour, wiki, mod docs | Low | Mechanics and formulas are not copyrightable. Keep a clean-room log. Avoid the "Kerbal" trademark in the product name. |
| Decompile Assembly-CSharp and translate to C++ | High | Direct copying of expression; EULA breach; re3 precedent with the same publisher. |
| Ship KSP assets | Very high | Clear infringement. |
| Load assets/configs from the user's own KSP install | Low to moderate | OpenMW/Daggerfall Unity model. No KSP-specific ruling exists. |

Recommendation: clean-room only, no decompiled references, new name, optional user-supplied asset loader as a later feature. Get counsel before any public release.

### 2.2 UE5 does not fix what makes KSP slow

What actually costs frame time in KSP1 (evidence is anecdotal; no public profiler breakdown exists):

- **Joint solver on one thread per connected vessel.** PhysX multithreads across separate bodies, not within one articulated tree. Struts and fuel lines create graph loops that cost more than their part count. Part-welding mods that collapse parts into one rigid body are the single most effective community performance fix, which tells you where the cost is.
- **Per-part MonoBehaviour updates and GC pressure** from hundreds of PartModules each tick. UNVERIFIED as a measured share, but consistent with KSPCommunityFixes gains (450-part craft in orbit: 46 to 91 FPS from caching raycasts and similar fixes alone).
- **Physics runs on all loaded parts of all craft in range**, rendered or not. KSP2 repeated this mistake.
- **Physics warp capped at 4x** because it multiplies the above.
- Terrain (PQS) and rendering are secondary. Scatterer and Parallax show the GPU side is tunable by mods; the CPU physics side is not.

What UE5 offers against that list:

| UE5 feature | Relevance to the KSP bottleneck |
|---|---|
| Chaos physics | Negative to unknown. Community reports instability past ~50 interacting bodies and slower scene queries than PhysX. A "10 to 100x slower" claim circulates but is UNVERIFIED. No benchmark of a 200 to 1000 body jointed tree was found. No official Jolt backend; UnrealJolt is a community bolt-on that coexists with Chaos rather than replacing it. |
| Large World Coordinates | Neutral. Still marked Beta in Epic docs. Transforms and Chaos run in double; rendering uses emulated double-float pairs with a documented performance cost; Niagara stays float. Floating-origin rebasing is still the right design. Solar-system scale orbital state should live in your own double-precision code regardless. |
| Nanite | Irrelevant to negative. Built for static geometry. Rocket parts are few, small, moving and detachable. |
| Lumen | Cost centre you do not need for a performance-first sim. |
| Mass Entity | Useful for many simple entities (debris, on-rails vessels), not for articulated physics bodies. |
| Sky Atmosphere | Favourable. Supports planet-centred atmospheres with configurable ground radius. |
| Spherical terrain | No native support. Third-party plugins exist (PlanetGen, WorldScape, SphereScape) with vendor-claimed walkable planets. Feasible but you will own a cubesphere quadtree either way. |

Where the performance actually comes from, engine-independent:

1. **Vessel as a reduced rigid-body model**, not one body per part. Options: single rigid body per vessel with a structural stress model for break-off (the "welded" approach), or a small number of bodies at articulation points (decouplers, docking ports, robotics) with parts aggregated between them. This is the 10x lever. KSA (RocketWerkz) uses Bepu physics in a custom framework and simulates mainly in doubles.
2. **Double-precision orbital state and Kepler on-rails propagation** outside the engine. Only the active physics bubble (KSP uses roughly 2.3 to 2.5 km) is integrated.
3. **Data-oriented part simulation**: struct-of-arrays part data, batched module ticks, no per-part heap allocation per frame.
4. **Camera-relative rendering** with the camera at origin (KSA's model, also Epic's own LWC guidance).

None of these require UE5. All of them are harder to do inside UE5's actor model than in plain C++.

---

## 3. Scope inventory

Stock KSP 1.12 with both DLCs is roughly 480 parts, 17 celestial bodies, and the following systems. Complexity ratings are judgment calls, not sourced.

| System | Size | Notes |
|---|---|---|
| Procedural quad-sphere terrain (PQS equivalent), biomes, oceans, scaled space vs local space | XL | Data-driven so planets can be modded (Kopernicus-style) adds more. |
| Atmospheres (pressure/temperature curves), Sky Atmosphere integration | L | |
| Orbital mechanics: patched conics, SOI transitions, maneuver nodes, orbit rendering | L | Pure math. Well documented publicly. Best agent target. |
| Vessel: part tree, attach nodes, staging, symmetry, decouplers, docking (tree merge), struts, crossfeed, resource priority | XL | Physics model choice (section 2.2) is the biggest architectural decision. |
| Part modules (engines, RCS, reaction wheels, command, SAS, solar, wheels, parachutes, docking, science, converters, antennas, etc.) | S to M each, L for wheels and converters | ~40+ module types. Highly parallelisable across agents once the module interface is fixed. |
| Stock aerodynamics (drag cubes, lift, Mach), reentry heating, thermal model | L to XL | FAR is the documented reference; stock model is observable. |
| Kerbals: EVA, EVA construction, IVA, jetpack, traits | L to XL | IVA is pure content cost. |
| Career: funds, reputation, science, tech tree, contracts, strategies, facility upgrades | L to XL | Largest systems-design effort. Defer. |
| Editors: VAB/SPH, categories, subassemblies, craft files, dV readout | M to L | |
| UI: map view, navball, staging, resource panel, tracking station, KSPedia, alarm clock | L | |
| CommNet | M to L | |
| Save format (ConfigNode .sfs/.craft) | M | Importing user .craft files is S to M; full .sfs compatibility is L. |
| Modding: ModuleManager-style cfg patching, plugin API | L to XL | ModuleManager-style patching is engine-neutral text processing. C# plugin compatibility is not realistic; UnrealSharp exists and is active but production maturity is UNVERIFIED. |
| Robotics, surface features, mission builder (DLC) | L to XL each | Defer. |
| Multiplayer | XL | Not stock. Out of scope. |

---

## 4. Effort calibration

Human-team precedents for engine reimplementations with user-supplied assets:

| Project | Start to 1.0 or feature-complete | Team |
|---|---|---|
| Daggerfall Unity | 2014/2015 to Dec 2023 | 50+ contributors |
| OpenRCT2 | 2014 to ongoing, playable within ~2 years via incremental replacement of the original EXE | ~250 contributors |
| OpenMW | 2008 to 0.50 in Nov 2025, still not 1.0 | large volunteer team |
| OpenGOAL | 2020 to all three games feature-complete Apr 2026 | decomp-based, not clean-room |

KSP has less art content than any of these (parts are simple meshes; terrain is procedural) but far more interacting simulation systems.

Agent-driven development evidence:

- GameEngineBench (July 2026): best agent configuration solved 55.5% of 110 real UE5 C++ tasks; 31 tasks unsolved by any configuration. Agents frequently compile but fail runtime behaviour. A clean build is a weak success signal.
- No public case study of a Claude Code or Codex-driven UE5 project was found.
- The constraint is verification throughput, not code generation. A UE5 build-and-test cycle is minutes; a plain C++ unit test cycle is seconds. This argues strongly for putting as much of the game as possible in an engine-agnostic core.

Estimates (mine, not sourced):

| Milestone | Human team of 2 to 4 | Agent-heavy, one human operator |
|---|---|---|
| Spike phase (section 6, phase 0) | 1 month | 2 to 4 weeks |
| Vertical slice: 20 parts, launch from a full-scale planet, reach orbit, land on a moon | 1 to 2 years | 3 to 6 months |
| KSP1 sandbox parity (no career) | 3 to 5 years | 1 to 2 years |
| KSP1 full parity incl. career and DLCs | 5 to 10 years | 2 to 4 years |

The agent-heavy column assumes a headless test harness exists from day one and that the simulation core is plain C++. Without those, expect the human-team numbers.

---

## 5. Recommended architecture

```
+------------------------------------------------------------+
|  UE5 shell (thin)                                           |
|  - camera-relative rendering, floating origin               |
|  - cubesphere terrain renderer, Sky Atmosphere              |
|  - VAB/SPH editor UI, flight HUD, map view (UMG/Slate)      |
|  - audio, input, settings                                   |
|  - OPTIONAL: Chaos or Jolt for the local physics bubble     |
+------------------------------+-----------------------------+
                               | C ABI / static lib
+------------------------------v-----------------------------+
|  Simulation core (engine-agnostic C++20, CMake, no UE deps) |
|  - double-precision body catalog, Kepler + patched conics   |
|  - time warp, on-rails propagation, SOI transitions         |
|  - vessel graph, staging, resource flow, part modules       |
|  - reduced rigid-body vessel model + structural stress      |
|  - aero + thermal models                                    |
|  - ConfigNode parser, ModuleManager-style patching          |
|  - deterministic, headless, unit + scenario tested          |
+------------------------------------------------------------+
```

Why this split:

- The core is where 80% of the logic and 100% of the performance-critical code lives, and it is where Opus is most productive: fast compile, deterministic tests, numeric validation against analytic orbits and published KSP values (dV tables, orbital periods, atmospheric curves).
- UE5 becomes replaceable. If the phase 0 spikes show Chaos or LWC are unacceptable, the same core can be hosted in Godot (double-precision build with Jolt), or a custom renderer, without rewriting the game.
- Blueprints are opaque to text-based agents. Keeping UE5 to a thin C++ shell with minimal Blueprint keeps the whole project agent-legible.

---

## 6. Phased plan and orchestration

Roles: Fable orchestrates, reviews architecture and gates phases. Haiku handles research and lookups. Opus 5.5 implements, with one agent per bounded module and a shared interface spec.

### Phase 0: Spikes (2 to 4 weeks). Kill the unknowns before writing game code.

1. **Physics spike.** In UE 5.6 or 5.7, build a 200-body and a 1000-body jointed tree. Measure Chaos stability and step time at fixed 50 Hz. Repeat with UnrealJolt. Repeat with a reduced model (one rigid body + stress graph). This decides the vessel physics model and whether Chaos is even in the picture.
2. **Scale spike.** Place a 600 km radius sphere (Kerbin scale) and an orbiting camera at 70 to 100 km altitude with LWC on. Verify no visual jitter with camera-relative rendering and a floating-origin shift every N km. Confirm Sky Atmosphere works at that radius.
3. **Iteration-loop spike.** Time a full UE5 project build, an incremental C++ change with Live Coding, a cook, and an automation test run on this machine. Set up a headless CMake build and test for the core. These numbers set agent throughput.
4. **Legal and naming.** Write the clean-room policy (no decompiled references; sources allowed: KSP wiki, forum, mod documentation, observed behaviour). Pick a non-infringing name.

**Gate:** proceed with UE5 as shell only if spike 1 shows an acceptable physics path and spike 3 shows an iteration loop under ~2 minutes for incremental changes. Otherwise re-evaluate the shell (Godot double-precision + Jolt is the leading alternative).

### Phase 1: Simulation core (3 to 4 months agent-heavy)

- Body catalog and Kerbol-system-equivalent data (17 bodies, published parameters).
- Kepler propagation, patched conics, SOI transitions, maneuver nodes. Validate against analytic solutions.
- Time warp state machine, physics bubble load/unload.
- ConfigNode parser; part and resource definitions; cfg patching.
- Vessel graph: attach nodes, staging, decouple, crossfeed and resource priority.
- Reduced rigid-body vessel model chosen in phase 0, thrust and mass properties, structural failure.
- Stock-like aero (drag cubes, lift) and a first-pass thermal model.
- Headless scenario tests: launch to orbit, Hohmann transfer, re-entry.

### Phase 2: UE5 shell (parallel with late phase 1)

- Camera-relative rendering and floating origin.
- Cubesphere quadtree terrain with heightmap + noise layers, LOD, collision in the bubble only.
- Sky Atmosphere per body, ocean plane.
- Vessel visualisation from the core's state; part meshes (placeholder art).
- VAB editor, navball, staging UI, map view.

### Phase 3: Vertical slice

A 20-part rocket: launch from a full-scale planet, reach orbit, transfer to a moon, land, return. This is the first point at which the "insanely performant" claim can be measured against KSP1 on the same hardware.

### Phase 4+: Breadth

Remaining part modules, more bodies, EVA, science, then career. Each module is an independent Opus task against the fixed core interface.

---

## 7. Hardware and tooling

- UE5 launcher build: ~100 GB. Source build: 400 to 500 GB, 32 GB RAM minimum, VS2022. Source build is only needed if Chaos solver changes are required; prefer launcher binaries.
- Fast NVMe for the UE project and DDC.
- Unreal MCP servers exist (remiphilippe/mcp-unreal for UE 5.7, flopperam/unreal-engine-mcp for 5.5+) for letting agents drive the editor. Maintenance status UNVERIFIED. Treat as a convenience, not a dependency.
- The core's CMake project needs no UE install, which lets most Opus work run without the editor at all.

---

## 8. Key risks

| Risk | Likelihood | Mitigation |
|---|---|---|
| Chaos cannot handle articulated vessels at acceptable cost | High | Phase 0 spike; reduced rigid-body model; Jolt or Godot fallback. |
| Take-Two legal action | Low if clean-room, high if decompiled | Clean-room policy, new name, no assets shipped, counsel before release. |
| Agent-generated code compiles but misbehaves | High (GameEngineBench) | Headless deterministic tests with numeric oracles as the acceptance criterion for every task. |
| UE5 iteration loop throttles agent throughput | Medium to high | Thin shell; most logic in the core. |
| Scope creep toward KSP1 parity before the slice works | High | Phase gates; career and DLC content deferred. |
| LWC Beta regressions across UE versions | Medium | Own double-precision state; UE only sees camera-relative floats. |
| Spiritual-successor competition (Kitten Space Agency, pre-alpha, custom engine, doubles, free) | Certain | Not a blocker, but it validates the "custom core, not Unity/Unreal" architecture and sets player expectations. |

---

## 9. Open questions for the user

1. Is UE5 a hard requirement, or is the goal "a modern, performant KSP-like" with UE5 as the default candidate? The architecture above works either way, but the phase 0 gate only matters if the answer is the latter.
2. Target: sandbox parity, or career too? Career roughly doubles the scope.
3. Should the project load the user's own KSP install (parts, cfgs, meshes) as an optional compatibility layer? It is the fastest route to content but adds moderate legal exposure.
4. Hardware available for the UE5 spikes (RAM, disk, GPU).

---

## 10. Source index

Research agents' primary URLs, grouped. All fetched 2026-10-08.

Legal and IP
- https://www.take2games.com/eula
- https://www.thegamer.com/take-two-gta-reverse-engineering-project-takedown/
- https://yro.slashdot.org/story/21/09/03/2013246/take-two-sues-enthusiasts-behind-gta-fan-projects-re3-revc
- https://www.thesixthaxis.com/2024/05/01/kerbal-space-program-2-studio-reportedly-shutting-down-but-take-two-promises-to-support-game/
- https://www.videogameschronicle.com/news/former-annapurna-staff-are-reportedly-taking-over-private-divisions-portfolio
- https://content2.uesp.net/wiki/OpenMW

UE5
- https://dev.epicgames.com/documentation/unreal-engine/large-world-coordinates-in-unreal-engine-5
- https://dev.epicgames.com/documentation/unreal-engine/large-world-coordinates-rendering-in-unreal-engine-5
- https://forums.unrealengine.com/t/chaos-physics-in-ue5-ea-and-beyond/231748?page=2
- https://forums.unrealengine.com/t/is-chaos-physics-performance-good-now/680327
- https://github.com/Yadhu-S/UnrealJolt
- https://dev.epicgames.com/documentation/unreal-engine/sky-atmosphere-component-in-unreal-engine
- https://www.fab.com/listings/96c36654-f244-4c75-a527-cfaaab2d5ac4 (PlanetGen)
- https://spherescape.dev/
- https://github.com/UnrealSharp/UnrealSharp
- https://docs.ficsit.app/satisfactory-modding/latest/Development/ModLoader/index.html

KSP architecture and performance
- https://github.com/KSPModdingLibs/KSPCommunityFixes
- https://forum.kerbalspaceprogram.com/topic/133444-will-part-count-still-be-an-issue-in-11
- https://forum.kerbalspaceprogram.com/topic/131437-could-someone-explain-the-max-physics-delta-time-per-frame/
- https://forum.kerbalspaceprogram.com/topic/207101-question-on-krakensbane
- https://forum.kerbalspaceprogram.com/topic/109654-physicsrails-distance-option
- https://forum.kerbalspaceprogram.com/topic/200602-developer-insights-4-%E2%80%93-ksp2-engineering
- https://forum.kerbalspaceprogram.com/topic/219210-ksp2-is-calculating-the-physics-of-all-parts-of-all-crafts-whether-they-are-rendered-or-not-reducing-performance-of-all-scenes-at-all-times/
- https://wiki.kerbalspaceprogram.com/wiki/Part
- https://wiki.kerbalspaceprogram.com/wiki/User:Greys/The_CFG_File_and_ConfigNodes
- https://github.com/sarbian/ModuleManager/wiki/Module%20Manager%20Syntax
- https://github.com/mockingbirdnest/Principia/wiki
- https://github.com/R-T-B/Kopernicus
- https://github.com/LGhassen/Deferred
- https://github.com/LGhassen/Scatterer/releases

Prior art and effort
- https://kittenspaceagency.wiki.gg/wiki/Frequently_Asked_Questions
- https://kittenspaceagency.wiki.gg/wiki/BRUTAL
- https://www.gamedeveloper.com/programming/rocketwerkz-ceo-predicts-frameworks-not-engines-will-be-future-of-game-development
- https://github.com/orbitersim/orbiter
- https://en.everybodywiki.com/Open_Space_Program
- https://github.com/Interkarma/daggerfall-unity/releases/tag/v1.0.0
- https://en.wikipedia.org/wiki/OpenRCT2
- https://en.wikipedia.org/wiki/OpenMW
- https://arxiv.org/abs/2607.03525 (GameEngineBench)
- https://dev.epicgames.com/documentation/unreal-engine/using-live-coding-to-recompile-unreal-engine-applications-at-runtime
- https://mcpservers.org/servers/remiphilippe/mcp-unreal
