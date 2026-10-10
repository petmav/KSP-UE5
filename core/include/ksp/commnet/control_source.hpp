// CommNet/VesselControlState.cs and CommNet/ICommNetControlSource.cs. (VesselControlState was ported by WP-24 inside module_command.hpp; WP-39
// moved it here so that ModuleCommand and the CommNet network can both include it.)
#pragma once

#include <string>

namespace ksp {

// CommNet/VesselControlState.cs ([Flags])
enum class VesselControlState : int {
    None = 0, Probe = 2, Kerbal = 4, Partial = 8, Full = 16,
    ProbeNone = 2, ProbePartial = 10, ProbeFull = 18, KerbalNone = 4, KerbalPartial = 12, KerbalFull = 20
};
inline VesselControlState operator&(VesselControlState a, VesselControlState b) {
    return static_cast<VesselControlState>(static_cast<int>(a) & static_cast<int>(b));
}
inline VesselControlState operator|(VesselControlState a, VesselControlState b) {
    return static_cast<VesselControlState>(static_cast<int>(a) | static_cast<int>(b));
}

namespace CommNet {

// ICommNetControlSource.cs
class ICommNetControlSource {
public:
    virtual ~ICommNetControlSource() = default;
    virtual std::string ControlSourceName() const = 0;  // `string name { get; }`
    virtual void UpdateNetwork() = 0;
    virtual VesselControlState GetControlSourceState() const = 0;
    virtual bool IsCommCapable() const = 0;
};

}  // namespace CommNet
}  // namespace ksp
