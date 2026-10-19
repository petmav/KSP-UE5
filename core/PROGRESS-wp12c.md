# PROGRESS WP-12c (branch wp12c)

- [x] Step 1a: extractor core/tools/mucollider/extract_scenery_colliders.py rewritten (schema /2) and run on KSP-dev (UnityPy 1.25.4): 53 PQSCity/PQSCity2 cities, KSC = 2795 objects / 1487 colliders (all facility levels, NestedPrefabSpawner "KSC/SpaceCenter" replayed), 706 meshes 16.8 MB.
- Findings: all scenery collider materials null -> Physics.defaultMaterial = "Metal" (PhysicsManager, ggm): 0.25/0.25/0, frictionCombine Maximum, bounceCombine Minimum. Terrain "Ground" (sharedassets9 2499): 0.9/0.9/0, Maximum, Minimum (Kerbin, Minmus, Moho, Eve, Laythe, Dres; other bodies null -> Metal).
- PSystemSetup (level9 pid 141) parsed (string[] typetree fix): stock launch sites = KSC LaunchPad/Runway facilities, Island_Airfield, Crater/MahiMahi/Cove/GlacierLake (PQSCity2, bundle "stock"); Woomerang/Desert are makinghistory_assets (DLC, not installed).
