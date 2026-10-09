// Small WP-07 enums: FoRModes (Assembly-CSharp/FoRModes.cs, navball frame of reference) and VesselTargetModes
// (Assembly-CSharp/VesselTargetModes.cs). Values and order as in KSP (saved and compared as ints).
#pragma once

#include <cstdint>

namespace ksp {

enum class FoRModes { SRF_NORTH, SRF_VEL, SRF_HDG, OBT_ABS, OBT_PLN, OBT_VEL, OBT_CTR, SHP_REL };

// Underlying type int32 and explicit values: the single definition (physics_globals.hpp includes this header).
enum class VesselTargetModes : int32_t { None = 0, Direction = 1, DirectionAndVelocity = 2, DirectionVelocityAndOrientation = 3 };

}  // namespace ksp
