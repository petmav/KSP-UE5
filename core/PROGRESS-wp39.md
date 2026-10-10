# PROGRESS WP-39 (CommNet and KerbNet), branch wp39 from main ad208205

Step 0 (done): read spec (CommNet/*, Network/*, ModuleDataTransmitter, ModuleKerbNetAccess, ModuleRelayEnabler, ModuleProbeControlPoint).
Plan: include/ksp/commnet/{network,comm_network,commnet_scene,comm_net_vessel,science_transmitter}.hpp, src/commnet/*.cpp,
modules module_data_transmitter / module_commnet_parts / module_kerbnet_access; tests/test_commnet.cpp; tools/wp39/commnet_reference.py.
Design: generic Net/Node/Link/Path are concrete base classes (virtual dispatch replaces the C# generics); Node owns a
ksp::dotnet::Dictionary (enumeration order = .NET Dictionary incl. LIFO free list); CommPath.Clear hides List.Clear (Net calls the base).
Vessel.connection: one pointer field added to Vessel.

Step 1 (done): all headers/sources written and compiling: src/commnet/{network,comm_network,commnet_scene,comm_net_vessel}.cpp,
src/modules/{module_commnet_parts,module_data_transmitter,module_kerbnet_access}.cpp; stand-in ModuleDataTransmitter removed from module_behaviours.cpp
(replaced by detail::RegisterCommNetModules); Vessel.connection added (vessel.hpp/.cpp GetControlLevel); ModuleCommand is an ICommNetControlSource
(VesselControlState moved to commnet/control_source.hpp); CommandEnvironment hooks default to the real scenario/connection.
Step 2 (done): tools/wp39/commnet_reference.py -> tests/data/wp39_reference.json (parts filtered to the in-game capture C9 part list + cfg_file).
Step 3 (in progress): tests/test_commnet.cpp debugging (most cases pass); then: check_cites, run_tests.ps1 -Fresh, gates, STATUS/README, WSL run.
