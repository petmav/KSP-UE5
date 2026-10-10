// WP-37: the stock contract templates (Assembly-CSharp/Contracts/Templates/*.cs): PlantFlag, CollectScience, PartTest, RecoverAsset,
// GrandTour, VesselRepairContract, OrbitalConstructionContract, RoverConstructionContract.
//
// Seams (see TemplateEnvironment below): the FinePrint parameter classes the last four build (WP-38 owns VesselSystemsParameter,
// VesselDestinationParameter, LocationAndSituationParameter, RepairPartParameter, ConstructionParameter, RoverWayPointParameter,
// CrewTraitParameter), the PartLoader / R&D tech tree view PartTest.Generate walks, the waypoint manager, the pre-built craft tables.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/contracts/contract.hpp"
#include "ksp/contracts/parameters.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {
class AvailablePart;
class ModuleTestSubject;
class Part;
class ProtoVessel;
class Vessel;
}  // namespace ksp

namespace ksp::Contracts::Templates {

// Contracts/Templates/PlantFlag.cs:11
class PlantFlag : public Contract {
public:
    std::string TypeName() const override { return "PlantFlag"; }
    CelestialBody* TargetBody() const { return targetBody; }                  // :17
    bool MeetRequirements() override;                                         // :19
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }  // :31
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                    // :36
    std::string GetSynopsys() override;                                       // :41
    std::string GetTitle() override;                                          // :46
    std::string MessageCompleted() override;                                  // :51
    void OnLoad(ConfigNode& node) override;                                   // :56
    void OnSave(ConfigNode& node) override;                                   // :64
    bool Generate() override;                                                 // :70
    std::string GetHashString() override;                                     // :129

protected:
    CelestialBody* targetBody = nullptr;
};

// Contracts/Templates/CollectScience.cs:10
class CollectScience : public Contract {
public:
    std::string TypeName() const override { return "CollectScience"; }
    CelestialBody* TargetBody() const { return targetBody; }                  // :17
    BodyLocation TargetLocation() const { return targetLocation; }            // :19
    bool MeetRequirements() override;                                         // :21
    std::vector<CelestialBody*> WeightBodies() override { return {targetBody}; }  // :29
    bool HasWeightBodies() override { return true; }
    std::string GetDescription() override;                                    // :34
    std::string GetSynopsys() override;                                       // :43
    std::string GetTitle() override;                                          // :54
    std::string MessageCompleted() override;                                  // :63
    void OnLoad(ConfigNode& node) override;                                   // :72
    void OnSave(ConfigNode& node) override;                                   // :84
    bool Generate() override;                                                 // :90

protected:
    CelestialBody* targetBody = nullptr;
    BodyLocation targetLocation = BodyLocation::Space;

private:
    bool GenerateContract_Orbit();                                            // :104
    bool GenerateContract_Surface();                                          // :140
};

// Contracts/Templates/PartTest.cs:12 (completed below)
class PartTest : public Contract {
public:
    std::string TypeName() const override { return "PartTest"; }
    std::string PartName;
    bool PartIsExperimental = false;
    bool Hauled() const { return hauled; }
protected:
    bool hauled = false;
};

// Contracts/Templates/RecoverAsset.cs (completed below)
class RecoverAsset : public Contract {
public:
    std::string TypeName() const override { return "RecoverAsset"; }
    void DiscoverAsset(Vessel* v);
};

}  // namespace ksp::Contracts::Templates
