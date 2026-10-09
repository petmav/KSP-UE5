// VesselRanges (VesselRanges.cs): per-situation load/unload/pack/unpack distances. PhysicsGlobals.VesselRangesDefault is
// the game-wide default (level0 prefab, then Physics.cfg `VesselRanges` node).
#pragma once

#include <cstdint>

namespace ksp {

class ConfigNode;

// Vessel.Situations (Vessel.cs:25), a [Flags]-style enum.
enum class VesselSituations : int32_t {
    LANDED = 1,
    SPLASHED = 2,
    PRELAUNCH = 4,
    FLYING = 8,
    SUB_ORBITAL = 0x10,
    ORBITING = 0x20,
    ESCAPING = 0x40,
    DOCKED = 0x80,
};
const char* ToString(VesselSituations s);  // Enum.ToString() (name; numeric text for unnamed values)

class VesselRanges {
public:
    // VesselRanges.Situation (VesselRanges.cs:7)
    struct Situation {
        float load = 0.0f;
        float unload = 0.0f;
        float pack = 0.0f;
        float unpack = 0.0f;

        Situation() = default;
        Situation(float load_, float unload_, float pack_, float unpack_)
            : load(load_), unload(unload_), pack(pack_), unpack(unpack_) {}
        void Load(const ConfigNode& node);  // VesselRanges.cs:33 (values scanned last to first; float.Parse throws)
        void Save(ConfigNode& node) const;  // VesselRanges.cs:57
    };

    // VesselRanges.cs:66-78 (C# field initialisers)
    Situation prelaunch{2250.0f, 2500.0f, 350.0f, 200.0f};
    Situation landed{2250.0f, 2500.0f, 350.0f, 200.0f};
    Situation splashed{2250.0f, 2500.0f, 350.0f, 200.0f};
    Situation flying{2250.0f, 22500.0f, 25000.0f, 2000.0f};
    Situation subOrbital{2250.0f, 15000.0f, 10000.0f, 200.0f};
    Situation orbit{2250.0f, 2500.0f, 350.0f, 200.0f};
    Situation escaping{2250.0f, 2500.0f, 350.0f, 200.0f};

    // VesselRanges.cs:95 GetSituationRanges (ORBITING, DOCKED and any other value -> orbit)
    Situation& GetSituationRanges(VesselSituations situation);
    const Situation& GetSituationRanges(VesselSituations situation) const;
    void Load(const ConfigNode& node);  // VesselRanges.cs:116 (child nodes scanned last to first)
    void Save(ConfigNode& node) const;  // VesselRanges.cs:149
};

}  // namespace ksp
