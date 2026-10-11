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
    void CalculateWaypoint(float minRadius, float maxRadius); // :140
    void OnRegister() override;                               // :154
    void OnUnregister() override;                             // :161
    bool CanBeCancelled() override { return true; }           // :168
    bool CanBeDeclined() override { return true; }            // :173
    std::string GetHashString() override;                     // :178
    std::string GetTitle() override;                          // :183
    std::string GetDescription() override;                    // :188
    std::string GetSynopsys() override;                       // :193
    std::string MessageCompleted() override;                  // :198
    void OnLoad(ConfigNode& node) override;                   // :203
    void OnSave(ConfigNode& node) override;                   // :254
    bool MeetRequirements() override;                         // :270
    void OnAccepted() override;                               // :284
    std::string CreateVesselName(const std::string& bodyName);  // :305
    void OnContractPreBuiltVesselSpawned(Vessel* vsl);        // :311
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :324
    void OnVesselDestroy(Vessel* vsl);                        // :332
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
    CelestialBody* TargetBody() const { return targetBody; }  // :44
    uint32_t ConstructionVesselId() const { return constructionVesselId; }  // :46
    const std::string& ConstructionPartName() const { return constructionPartName; }  // :48
    double OrbitEccentricity() const { return orbitEccentricity; }  // :50
    double OrbitAltitudeFactor() const { return orbitAltitudeFactor; }  // :52
    double OrbitInclinationFactor() const { return orbitInclinationFactor; }  // :54
    const std::string& VesselName() const { return vesselName; }  // :56
    bool Generate() override;                                 // :58
    void OnRegister() override;                               // :201
    void OnUnregister() override;                             // :208
    bool CanBeCancelled() override { return true; }           // :216
    bool CanBeDeclined() override { return true; }            // :221
    std::string GetHashString() override;                     // :226
    std::string GetTitle() override;                          // :231
    std::string GetDescription() override;                    // :236
    std::string GetSynopsys() override;                       // :241
    std::string MessageCompleted() override;                  // :256
    void OnLoad(ConfigNode& node) override;                   // :261
    void OnSave(ConfigNode& node) override;                   // :325
    bool MeetRequirements() override;                         // :347
    void OnAccepted() override;                               // :357
    std::string CreateVesselName(const std::string& bodyName);  // :374
    void OnContractPreBuiltVesselSpawned(Vessel* vsl);        // :380
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :392
    void OnVesselDestroy(Vessel* vsl);                        // :400
    void SetupRenderer();                                     // :408
    void CleanupRenderer();                                   // :416
};

// Contracts/Templates/VesselRepairContract.cs:14
class VesselRepairContract : public Contract {
public:
    std::string TypeName() const override { return "VesselRepairContract"; }
    CelestialBody* targetBody = nullptr;                      // :16
    std::shared_ptr<PreBuiltCraftDefinition> repairCraftDefinition;  // :18
    double craftStartLatitude = 0.0;                          // :20
    double craftStartLongitude = 0.0;                         // :22
    uint32_t repairVesselId = 0;                              // :24
    uint32_t repairPartId = 0;                                // :26
    double orbitEccentricity = 0.0;                           // :28
    double orbitAltitudeFactor = 0.5;                         // :30
    double orbitInclinationFactor = 0.5;                      // :32
    std::string vesselName;                                   // :34
    bool orbitRendererSet = false;                            // orbitRenderer != null
    std::unique_ptr<Orbit> orbit;                             // :38
    bool spawnOnGround = false;                               // :40
    bool spawnInOrbit = false;                                // :42
    std::string repairPartName;                               // :44
    bool waitingForVslSpawn = false;                          // :46
    const AvailablePart* repairPart = nullptr;                // :48
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
    void OnRegister() override;                               // :235
    void OnUnregister() override;                             // :242
    bool CanBeCancelled() override { return true; }           // :250
    bool CanBeDeclined() override { return true; }            // :255
    std::string GetHashString() override;                     // :260
    std::string GetTitle() override;                          // :265
    std::string GetDescription() override;                    // :270
    std::string GetSynopsys() override;                       // :275
    std::string MessageCompleted() override;                  // :292
    void OnLoad(ConfigNode& node) override;                   // :297
    void OnSave(ConfigNode& node) override;                   // :431
    bool MeetRequirements() override;                         // :466
    void OnAccepted() override;                               // :476
    std::string CreateVesselName(const std::string& bodyName);  // :511
    void OnContractPreBuiltVesselSpawned(Vessel* vsl);        // :517
    void VesselIdChanged(uint32_t oldId, uint32_t newId);     // :558
    void OnVesselDestroy(Vessel* vsl);                        // :566
    void SetupRenderer();                                     // :574
    void CleanupRenderer();                                   // :582
};

}  // namespace ksp::Contracts::Templates
