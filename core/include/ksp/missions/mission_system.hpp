// WP-54c: Assembly-CSharp/Expansions/Missions/Runtime/MissionSystem.cs (Expansions.Missions.Runtime.MissionSystem), the ScenarioModule that owns the
// running missions. This header holds the part of the class the node library reads (the static mission list); the rest is added by mission_system.cpp.
#pragma once

#include <vector>

namespace ksp::Expansions::Missions {
class Mission;
}

namespace ksp::Expansions::Missions::Runtime {

class MissionSystem {
public:
    // MissionSystem.cs:@missions (public static List<Mission>)
    static inline std::vector<Mission*> missions;
};

}  // namespace ksp::Expansions::Missions::Runtime
