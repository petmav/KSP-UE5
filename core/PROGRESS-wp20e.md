# PROGRESS WP-20e: contact, landing and water (branch `wp20e`, from origin/main cd5e4df4)

Implementer package (no subagents). Scope: PHYSICS-SPIKE.md section 7 row WP-20e on the WP-20b binding, the WP-19b bubble and the WP-20d tick:
contacts -> Part.OnCollisionEnter / Stay / Exit, LandedCollisionChecks, HandleCollision crash tolerance (Part.cs:3355-3655), Part.checkLanded /
checkSplashed, Vessel landed / splashed / situation transitions and landedAt, the landed RB anchor (Vessel.cs:1105-1183, the pad rocking), the
vessel ground probes (GetHeightFromTerrain, CheckGroundCollision + getLowestPoint, Vessel.cs:2493-2673), CollisionEnhancer (anti-tunnelling),
PartBuoyancy (water: displacement, submersion, buoyancy, water drag through rb.drag / angularDrag, splashdown crash), TerrainColliders hosted per
celestial body in the bubble (WP-12b's seam 3), vessel-vessel collisions; SAS / FeedInputFeed in the tick if feasible, else a labelled seam.
Not touched: decompiled/KSP-Decompiled (read only).

## Plan
1. PhysicsWorld (glue): contact events per sub-shape pair (Enter / Stay / Exit with the pre-solve body kinematics), a fixed-to-world
   constraint (the RB anchor) with its force; WP-20c's StaticContacts unchanged.
2. Part.cs ports (src/parts/part_contact.cpp): OnCollisionEnter / LandedCollisionChecks / OnCollisionExit / CheckOtherGroundContact /
   CheckCollision / HandleCollision / OnCollisionStay / ResetCollisions / checkLanded / checkSplashed / explode over a `Collision` record.
3. Vessel.cs ports: AddRBAnchor / ResetRBAnchor / OnJointBreak (LateUpdate), CheckGroundCollision / getLowestPoint, crashObjectName.
4. PartBuoyancy.cs and CollisionEnhancer.cs ports (Mono float rule; seams for the Unity state).
5. VesselPhysics: rb.drag (PhysX linear damping per part), part lookup by index, contact bookkeeping.
6. ContactHost in the bubble: static collider registry (layer / tag / PQ body), contact dispatch after the step, buoyancy + enhancer slots,
   anchors, vessel probes, TerrainColliders per body (origin / rotation from the bubble's frame, rebuilt on frame changes).
7. Gates: tools/wp20e reference (independent Python, 0 ULP for the float statements); scenarios: drop on terrain (settle, LANDED, landedAt),
   crash at 0.9x / 1.1x crashTolerance, high-speed crash (CollisionEnhancer), splashdown floats at KSP's equilibrium (within 2 % of analytic),
   vessel-vessel collision, L2-AS on the pad without rocking (anchor), terrain colliders from the real Kerbin PQS.
8. Docs: STATUS row 55 + section, README, NEXT-STEPS, check_cites, random order 11 / 27, ctest, PR.

## Log
- start: read CLAUDE.md, README, STATUS WP-20b / 20c / 20d / 19b / 12b, PHYSICS-SPIKE.md 5-7, PROGRESS-wp20d, PHYSICS-ENGINE-REVIEW.md 1.2 /
  3.6 / 7.4; C#: Part.cs:3355-3745 (collisions, checkLanded / Splashed), :4373 explode, Vessel.cs:1015-1183 (checkLanded, anchor, SetLandedAt),
  :2356-2730 (rails, CheckGroundCollision, getLowestPoint, heights), :3055 LateUpdate, :3133-3195 (situation), PartBuoyancy.cs (whole),
  CollisionEnhancer.cs (whole). Worktree core/.worktrees/wp20e from origin/main cd5e4df4; baseline build 208 s.
