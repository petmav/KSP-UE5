# serialized (asset extraction)

Produced by `harness/tools/extract_assets.py` (run by `run.ps1` when missing or with `-Extract`). These are the values
Unity deserializes into KSP's MonoBehaviours **before any KSP code runs**; the C# harness overlays them onto freshly
constructed objects (ctor first, then fields, as Unity does) and then calls KSP's own Awake/Start logic.

| file | source asset | contents |
|---|---|---|
| `celestial_bodies.serialized.json` | `KSP_x64_Data/sharedassets9.assets` | PSystem tree (spawn order, parent, flightGlobalsIndex) and, per body, every serialized field of `CelestialBody` (FloatCurves with raw keyframes, wrap modes, weights) and of its `OrbitDriver` (incl. the serialized `Orbit`) |
| `physicsglobals.serialized.json` | `KSP_x64_Data/level0` | every serialized field of the `PhysicsGlobals` prefab (later partly overridden by Physics.cfg) |
| `scene_singletons.serialized.json` | `KSP_x64_Data/level9` | `Planetarium`, `FlightGlobals`, `PSystemManager` serialized fields |

How: UnityPy 1.25 reads the asset files; the player build has no type trees, so the MonoBehaviour layouts are generated
from KSP's shipped IL (`lib/Assembly-CSharp.de4dot-original.dll`) plus the Unity/firstpass DLLs and the game's Mono BCL
with TypeTreeGeneratorAPI. Floats are exact (float32 values widened to double; doubles as stored). Asset file SHA-256s
are in each file's `provenance`.

Trust: **serialized** (read from the shipped data, not computed). Note many doubles are float-derived decimals, e.g.
eccentricity 0.200000002980232, Kerbin sma 13599840256, atmosphereAdiabaticIndex 1.39999997615814; a C++ catalog must
use these exact values, not the "clean" wiki numbers.
