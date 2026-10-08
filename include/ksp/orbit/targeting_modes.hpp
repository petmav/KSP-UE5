// Small WP-07 enums: FoRModes (Assembly-CSharp/FoRModes.cs, navball frame of reference) and VesselTargetModes
// (Assembly-CSharp/VesselTargetModes.cs). Values and order as in KSP (saved and compared as ints).
#pragma once

namespace ksp {

enum class FoRModes { SRF_NORTH, SRF_VEL, SRF_HDG, OBT_ABS, OBT_PLN, OBT_VEL, OBT_CTR, SHP_REL };

enum class VesselTargetModes { None, Direction, DirectionAndVelocity, DirectionVelocityAndOrientation };

}  // namespace ksp
