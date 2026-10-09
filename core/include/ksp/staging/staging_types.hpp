// WP-16 small staging types: PreferredInitialStage.cs, IStageSeparator.cs, IStageSeparatorChild.cs, IActivateOnDecouple.cs,
// IAirstreamShield.cs (as far as ModuleDecouplerBase implements it) and the DefaultIcons names a part's `stagingIcon` must parse to.
// (KSPActionGroup.cs is save/save_enums.hpp (WP-04), KSPActionParam.cs / KSPActionType.cs modules/base_event.hpp, ModifierStagingSituation.cs
// modules/part_module.hpp (WP-06); all of them complete for WP-16.)
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ksp {

class Part;
class Vessel;

// PreferredInitialStage.cs (AvailablePart::preferredStage holds the value as an int)
enum class PreferredInitialStage : int { FIRST, LAST, PARACHUTESTAGE, DEFAULT };

// IStageSeparator.cs
class IStageSeparator {
public:
    virtual ~IStageSeparator() = default;
    virtual int GetStageIndex(int fallback) = 0;
};

// IStageSeparatorChild.cs
class IStageSeparatorChild {
public:
    virtual ~IStageSeparatorChild() = default;
    virtual bool PartDetaches(std::vector<Part*>& decoupledChildParts) = 0;
    virtual bool IsEnginePlate() = 0;
};

// IActivateOnDecouple.cs
class IActivateOnDecouple {
public:
    virtual ~IActivateOnDecouple() = default;
    virtual void DecoupleAction(const std::string& nodeName, bool weDecouple) = 0;
};

// IAirstreamShield.cs
class IAirstreamShield {
public:
    virtual ~IAirstreamShield() = default;
    virtual bool ClosedAndLocked() = 0;
    virtual Vessel* GetVessel() = 0;
    virtual Part* GetPart() = 0;
};

// DefaultIcons.cs: the members Part.Start parses `stagingIcon` into (Enum.Parse; an unknown name logs an error and clears the field)
bool IsDefaultIconName(std::string_view name);

}  // namespace ksp
