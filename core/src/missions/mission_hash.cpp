// WP-54c: the GetHashCode() overrides of the Making History data classes (ScoreRange, VesselPartIDPair, MissionInstructor, MissionBiome, MissionOrbit,
// VesselGroundLocation, SurfaceArea, SurfaceVolume, MissionKerbal, DynamicModule, DynamicModuleList). See mission_hash.hpp for the System.*.GetHashCode rules.
// Cite placeholders `File.cs:@Name[#k]` are resolved by tools/wp55/check_cites.py --resolve.
#include <functional>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/missions/dynamic_module.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_actions.hpp"
#include "ksp/missions/mission_data.hpp"
#include "ksp/missions/mission_hash.hpp"
#include "ksp/missions/mission_score.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/util/misc_util.hpp"

namespace ksp::Expansions::Missions {

namespace {
// string.GetHashCode_Net35() (a null string is a NullReferenceException)
int32_t Net35(const std::string& s) { return StringExtensions::GetHashCode_Net35(s); }
CelestialBody* Need(CelestialBody* b) {
    if (b == nullptr) throw NullReferenceException();
    return b;
}
}  // namespace

// object.GetHashCode() / UnityEngine.Object.GetHashCode()
int32_t MissionDataEnvironment::ObjectHash(const void* p) {
    if (objectHashCode) return objectHashCode(p);
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(p) & 0xFFFFFFFFu);
}

// ScoreRange.cs:@GetHashCode
int32_t ScoreRange::GetHashCode() const { return Hash::Of(minRange) ^ Hash::Of(maxRange) ^ Hash::Of(score); }

// VesselPartIDPair.cs:@GetHashCode
int32_t VesselPartIDPair::GetHashCode() const { return Hash::Of(VesselID) ^ Hash::Of(partID); }

// MissionInstructor.cs:@GetHashCode
int32_t MissionInstructor::GetHashCode() const { return Net35(instructorName) ^ Hash::Of(vintageSuit); }

// DynamicModule.cs:@GetHashCode
int32_t DynamicModule::GetHashCode() const { return MonoStringHash(name_); }

// DynamicModuleList.cs:@GetHashCode (activeModules.GetHashCode(): the reference hash of the List object)
int32_t DynamicModuleList::GetHashCode() const { return MissionDataEnvironment::ObjectHash(&activeModules); }

namespace Editor {
// MissionBiome.cs:@GetHashCode
int32_t MissionBiome::GetHashCode() const { return Net35(biomeName) ^ MissionDataEnvironment::ObjectHash(Need(body)); }
}  // namespace Editor

// MissionOrbit.cs:@GetHashCode
int32_t MissionOrbit::GetHashCode() const {
    return Hash::Of(ReferenceBodyIndex) ^ Hash::Of(semiMajorAxis) ^ Hash::Of(eccentricity) ^ Hash::Of(inclination) ^ Hash::Of(LAN) ^ Hash::Of(argOfPeriapsis) ^ Hash::Of(epoch) ^
           Hash::Of(meanAnomalyAtEpoch);
}

// VesselGroundLocation.cs:@GetHashCode
int32_t VesselGroundLocation::GetHashCode() const {
    return MissionDataEnvironment::ObjectHash(Need(targetBody)) ^ Hash::Of(altitude) ^ Hash::Of(latitude) ^ Hash::Of(longitude) ^
           MissionDataEnvironment::ObjectHash(rotation.get());  // MissionQuaternion does not override GetHashCode
}

// SurfaceArea.cs:@GetHashCode
int32_t SurfaceArea::GetHashCode() const { return Net35(Need(body)->name()) ^ Hash::Of(latitude) ^ Hash::Of(longitude) ^ Hash::Of(radius); }

// SurfaceVolume.cs:@GetHashCode
int32_t SurfaceVolume::GetHashCode() const {
    return MissionDataEnvironment::ObjectHash(Need(body)) ^ Hash::Of(latitude) ^ Hash::Of(longitude) ^ Hash::Of(radius) ^ Hash::Of(shape) ^ Hash::Of(heightSphere) ^
           Hash::Of(heightConeMin) ^ Hash::Of(heightConeMax);
}

// MissionKerbal.cs:@GetHashCode
int32_t MissionKerbal::GetHashCode() const {
    if (kerbal == nullptr) throw NullReferenceException();
    return Hash::Of(typeToShow) ^ kerbal->GetHashCode();
}

}  // namespace ksp::Expansions::Missions
