// WP-38 slice 3 (the WP-37 deferred templates): Contracts/Templates/{VesselRepairContract,OrbitalConstructionContract,RoverConstructionContract}.cs.
//
// The pre-built craft come from ContractDefs::PreBuiltCraftDefs (fineprint/prebuilt.hpp) and ContractDefs::constructionParts; spawning the vessel
// (ContractSystem.SpawnPreBuiltShip -> ShipConstruction + ProtoVessel) is PreBuiltEnvironment::spawnPreBuiltShip. `new KSPRandom()` (the unseeded
// generator of Generate) is ContractsEnvironment::newRandomSeed. The map renderer is FinePrintEnvironment::setupOrbitRenderer.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/contracts/contract.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/fineprint/contracts.hpp"
#include "ksp/fineprint/fineprint.hpp"
#include "ksp/fineprint/parameters.hpp"
#include "ksp/fineprint/prebuilt.hpp"
#include "ksp/fineprint/utilities.hpp"
#include "ksp/orbit/orbit.hpp"

namespace ksp::Contracts::Templates {

using ksp::Contracts::Lite::PreBuiltCraftDefinition;

// Contracts/Templates/RoverConstructionContract.cs:12
class RoverConstructionContract : public Contract, public FinePrint::IUpdateWaypoints {
public:
    std::string TypeName() const override { return "RoverConstructionContract"; }
    CelestialBody* targetBody = nullptr;                      // :14
    double craftStartLatitude = 0.0;                          // :16
    double craftStartLongitude = 0.0;                         // :18
    double craftEndLatitude = 0.0;                            // :20
    double craftEndLongitude = 0.0;                           // :22
    uint32_t roverVesselId = 0;                               // :24
    std::shared_ptr<PreBuiltCraftDefinition> roverCraftDefinition;  // :26
    bool waitingForVslSpawn = false;                          // :28
    std::vector<std::string> VesselNameStrings = {"#autoLOC_6002628", "#autoLOC_6002629", "#autoLOC_6002630", "#autoLOC_6002631", "#autoLOC_6002632",
                                                  "#autoLOC_6002633", "#autoLOC_6002634", "#autoLOC_6002635", "#autoLOC_6002636"};  // :30
    CelestialBody* TargetBody() const { return targetBody; }  // :32
    double CraftStartLatitude() const { return craftStartLatitude; }      // :34
    double CraftStartLongitude() const { return craftStartLongitude; }    // :36
    double CraftEndLatitude() const { return craftEndLatitude; }          // :38
    double CraftEndLongitude() const { return craftEndLongitude; }        // :40
    uint32_t RoverVesselId() const { return roverVesselId; }              // :42
    bool Generate() override;                                 // :44
    void CalculateWaypoint(float minRadius, float maxRadius); // :147
    void OnRegister() override;                               // :161
    void OnUnregister() override;                             // :168
    bool CanBeCancelled() override { return true; }           // :175
    bool CanBeDeclined() override { return true; }            // :180
    std::string GetHashString() override;                     // :185
    std::string GetTitle() override;                          // :190
    std::string GetDescription() override;                    // :195
    std::string GetSynopsys() override;                       // :200
    std::string MessageCompleted() override;                  // :205
    void OnLoad(ConfigNode& node) override;                   // :210
    void OnSave(ConfigNode& node) override;                   // :253
    bool MeetRequirements() override;                         // :270
    void OnAccepted() override;                               // :283
    std::string CreateVesselName(const std::string& bodyName);  // :302
    void OnContractPreBuiltVesselSpawned(Vessel* vsl);        // :308
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :321
    void OnVesselDestroy(Vessel* vsl);                        // :329
};

// Contracts/Templates/OrbitalConstructionContract.cs:13
class OrbitalConstructionContract : public Contract, public FinePrint::IUpdateWaypoints {
public:
    std::string TypeName() const override { return "OrbitalConstructionContract"; }
    CelestialBody* targetBody = nullptr;                      // :15
    uint32_t constructionVesselId = 0;                        // :17
    std::string constructionPartName;                         // :19
    double orbitEccentricity = 0.0;                           // :21
    double orbitAltitudeFactor = 0.5;                         // :23
    double orbitInclinationFactor = 0.5;                      // :25
    std::string vesselName;                                   // :27
    bool orbitRendererSet = false;                            // orbitRenderer != null
    std::unique_ptr<Orbit> orbit;                             // :31
    const AvailablePart* constructionPart = nullptr;          // :33
    bool waitingForVslSpawn = false;                          // :35
    std::shared_ptr<PreBuiltCraftDefinition> craftDefinition; // :37
    std::vector<std::string> VesselNameStrings = {"#autoLOC_6002605", "#autoLOC_6002606", "#autoLOC_6002607", "#autoLOC_6002608", "#autoLOC_6002609", "#autoLOC_6002610",
                                                  "#autoLOC_6002611", "#autoLOC_6002612", "#autoLOC_6002613", "#autoLOC_6002614", "#autoLOC_6002615", "#autoLOC_6002616"};  // :39
    CelestialBody* TargetBody() const { return targetBody; }  // :45
    uint32_t ConstructionVesselId() const { return constructionVesselId; }  // :47
    const std::string& ConstructionPartName() const { return constructionPartName; }  // :49
    double OrbitEccentricity() const { return orbitEccentricity; }  // :51
    double OrbitAltitudeFactor() const { return orbitAltitudeFactor; }  // :53
    double OrbitInclinationFactor() const { return orbitInclinationFactor; }  // :55
    const std::string& VesselName() const { return vesselName; }  // :57
    bool Generate() override;                                 // :59
    void OnRegister() override;                               // :209
    void OnUnregister() override;                             // :216
    bool CanBeCancelled() override { return true; }           // :224
    bool CanBeDeclined() override { return true; }            // :229
    std::string GetHashString() override;                     // :234
    std::string GetTitle() override;                          // :239
    std::string GetDescription() override;                    // :244
    std::string GetSynopsys() override;                       // :249
    std::string MessageCompleted() override;                  // :263
    void OnLoad(ConfigNode& node) override;                   // :268
    void OnSave(ConfigNode& node) override;                   // :317
    bool MeetRequirements() override;                         // :338
    void OnAccepted() override;                               // :347
    std::string CreateVesselName(const std::string& bodyName);  // :363
    void OnContractPreBuiltVesselSpawned(Vessel* vsl);        // :369
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :382
    void OnVesselDestroy(Vessel* vsl);                        // :390
    void SetupRenderer();                                     // :398
    void CleanupRenderer();                                   // :406
};

// Contracts/Templates/VesselRepairContract.cs:14
class VesselRepairContract : public Contract {
public:
    std::string TypeName() const override { return "VesselRepairContract"; }
    CelestialBody* targetBody = nullptr;                      // :15
    std::shared_ptr<PreBuiltCraftDefinition> repairCraftDefinition;  // :17
    double craftStartLatitude = 0.0;                          // :19
    double craftStartLongitude = 0.0;                         // :21
    uint32_t repairVesselId = 0;                              // :23
    uint32_t repairPartId = 0;                                // :25
    double orbitEccentricity = 0.0;                           // :27
    double orbitAltitudeFactor = 0.5;                         // :29
    double orbitInclinationFactor = 0.5;                      // :31
    std::string vesselName;                                   // :33
    bool orbitRendererSet = false;                            // orbitRenderer != null
    std::unique_ptr<Orbit> orbit;                             // :37
    bool spawnOnGround = false;                               // :39
    bool spawnInOrbit = false;                                // :41
    std::string repairPartName;                               // :43
    bool waitingForVslSpawn = false;                          // :45
    const AvailablePart* repairPart = nullptr;                // :47
    std::vector<std::string> VesselNameStrings = {"#autoLOC_6002581", "#autoLOC_6002582", "#autoLOC_6002583", "#autoLOC_6002584", "#autoLOC_6002585", "#autoLOC_6002586",
                                                  "#autoLOC_6002587", "#autoLOC_6002588", "#autoLOC_6002589", "#autoLOC_6002590", "#autoLOC_6002591"};  // :50
    CelestialBody* TargetBody() const { return targetBody; }  // :55
    double CraftStartLatitude() const { return craftStartLatitude; }   // :57
    double CraftStartLongitude() const { return craftStartLongitude; } // :59
    uint32_t RepairVesselId() const { return repairVesselId; }         // :61
    uint32_t RepairPartId() const { return repairPartId; }             // :63
    double OrbitEccentricity() const { return orbitEccentricity; }     // :65
    double OrbitAltitudeFactor() const { return orbitAltitudeFactor; } // :67
    double OrbitInclinationFactor() const { return orbitInclinationFactor; }  // :69
    const std::string& VesselName() const { return vesselName; }       // :71
    const std::string& RepairPartName() const { return repairPartName; }  // :73
    bool Generate() override;                                 // :75
    void OnRegister() override;                               // :272
    void OnUnregister() override;                             // :279
    bool CanBeCancelled() override { return true; }           // :287
    bool CanBeDeclined() override { return true; }            // :292
    std::string GetHashString() override;                     // :297
    std::string GetTitle() override;                          // :302
    std::string GetDescription() override;                    // :307
    std::string GetSynopsys() override;                       // :312
    std::string MessageCompleted() override;                  // :326
    void OnLoad(ConfigNode& node) override;                   // :331
    void OnSave(ConfigNode& node) override;                   // :455
    bool MeetRequirements() override;                         // :486
    void OnAccepted() override;                               // :496
    std::string CreateVesselName(const std::string& bodyName);  // :524
    void OnContractPreBuiltVesselSpawned(Vessel* vsl);        // :530
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :570
    void OnVesselDestroy(Vessel* vsl);                        // :578
    void SetupRenderer();                                     // :586
    void CleanupRenderer();                                   // :594
};

}  // namespace ksp::Contracts::Templates
