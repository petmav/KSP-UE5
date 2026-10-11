// Jolt spike: static terrain colliders around the launch pad from Kerbin's real PQS height stack (WP-11 port), one Jolt
// MeshShape body per quad, positioned in the PHYSICS FRAME (body-fixed, double) -- the WP-12 collider interface in miniature:
// a quad is (centre: double, vertices relative to the centre: float, triangles). KSP's scene census during the profiled flights
// had 917 static MeshColliders (mostly PQS quads, PROFILE.md section 6).
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Math/DVec3.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <vector>

#include "jolt_world.hpp"

namespace spike {

class KspContext;

struct SurfaceFrame {
    JPH::DVec3 up, east, north;  // unit vectors at the pad (body-fixed)
    double radius = 0.0;         // PQS surface radius at the pad
};

struct TerrainConfig {
    int quadsPerSide = 30;   // 30 x 30 = 900 quads
    int vertsPerSide = 15;   // PQS quads are 15 x 15 vertices (PQS.cache: 225 vertices per quad)
    double quadSize = 50.0;  // m
};

struct TerrainPatch {
    std::vector<JPH::BodyID> bodies;
    int triangles = 0;
    double heightEvalMs = 0.0, buildMs = 0.0;
    double minHeight = 0.0, maxHeight = 0.0;  // terrain height (m above the sphere radius) over the patch
};

SurfaceFrame PadFrame(KspContext& ctx);
TerrainPatch BuildTerrainPatch(World& world, KspContext& ctx, const SurfaceFrame& f, const JPH::DVec3& frameOrigin,
                               const TerrainConfig& cfg);
// KSP's FloatingOrigin moves every static collider too: measure what that costs in Jolt
void ShiftStatics(World& world, const TerrainPatch& patch, JPH::Vec3Arg delta);

}  // namespace spike
