# PROGRESS WP-15 (resource flow)

Branch wp15 (from wp13 688c5711). Notes written after every major step.

- [x] read CLAUDE/NEXT-STEPS/CLOUD-TASKS/README/STATUS; read PartSet, ResourceMap, ModuleResourceHandler, graph files, Part.cs resource members
- baseline build running: log /tmp/claude-0/wp15/baseline_build.log
- Design (decided):
  * PartResource keeps public fields (flowState/flowMode = C# _flowState/_flowMode); add SetFlowState/SetFlowMode (event-firing properties), CanProvide, Flowing, SetInfo, GetInfo
  * PartResourceList class (DictionaryValueList<int,PartResource*> + owned storage) replaces std::vector<PartResource> in PartPrefab::Resources and Part::simulationResources_
  * seam to WP-14: ksp/resources/resource_flow_host.hpp: IResourceVessel (parts + RequestResource + GetConnectedResourceTotals) and VesselResourceSets (Vessel.cs:3913-4040 PartSet owner); mapping Vessel*/ShipConstruct* -> IResourceVessel* via ResourceFlowEnvironment hook
  * HashSet<Part> = insertion ordered (dotnet HashSet semantics: entries array + LIFO free list)
  * planetary resource layer (ResourceMap/Cache/Scenario/GameSettings/NoiseGenerator) ported as plain classes

## Step 1 (written, not yet compiled): resource list, flow graph, PartSet, Part flow members
- new: include/ksp/dotnet/hash_set.hpp, include/ksp/resources/{resource_types,part_resource_list,flow_graph,part_set,resource_flow_host}.hpp
  src/resources/{part_resource_list,flow_graph,part_set,resource_flow_host}.cpp, src/parts/part_resource_flow.cpp
- migrated: PartResource (SetFlowState/SetFlowMode/CanProvide/Flowing...), PartPrefab::Resources + Part::simulationResources_ -> PartResourceList, part.cpp resource block, Instantiate/OnCopy
- baseline (before changes): 21/21 ctest pass (build 343 s cold)
- Step 1 compiled; merged wp13 (ae3e81fa) clean; full unit suite green (173 cases). C# quirks found: PartResourceList 4-arg ctor never sets `simulationSet` (sim twins loaded by Add(node) have simulationResource=false until refresh); Part.OnCopy removes from Resources only (stale sim twin) -> two WP-13 test expectations corrected to the C# behaviour.
- TODO next: modules (ModuleResource/Handler/AutoShift/ToggleCrossfeed/FuelLine/EVA), broker/converter, planetary layer (map/cache/scenario/settings/noise), tests/test_resource_flow.cpp, STATUS/README
