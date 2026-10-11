#include "terrain_patch.hpp"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <stdexcept>
#include <tuple>

#include "ksp_context.hpp"

namespace spike {

using namespace JPH;
using Clock = std::chrono::steady_clock;

SurfaceFrame PadFrame(KspContext& ctx) {
    SurfaceFrame f;
    const ksp::Vector3d n = KspContext::SurfaceNormal(kPadLat, kPadLon);
    f.up = DVec3(n.x, n.y, n.z).Normalized();
    const DVec3 polar(0.0, 1.0, 0.0);  // body-fixed Y is the rotation axis (GetRelSurfaceNVector is .xzy)
    f.north = (polar - f.up * polar.Dot(f.up)).Normalized();
    f.east = f.up.Cross(f.north);
    f.radius = ctx.SurfaceRadius(ksp::Vector3d(f.up.GetX(), f.up.GetY(), f.up.GetZ()));
    return f;
}

TerrainPatch BuildTerrainPatch(World& world, KspContext& ctx, const SurfaceFrame& f, const DVec3& frameOrigin, const TerrainConfig& cfg) {
    TerrainPatch out;
    const int Q = cfg.quadsPerSide, V = cfg.vertsPerSide;
    const int G = Q * (V - 1) + 1;  // shared vertex grid
    const double step = cfg.quadSize / (V - 1);
    const double half = 0.5 * Q * cfg.quadSize;
    const DVec3 padPoint = f.up * f.radius;
    // ---- heights (WP-11 PQS.GetSurfaceHeight with overrideQuadBuildCheck: the vessel-positioning variant); cached per layout
    auto t0 = Clock::now();
    static std::map<std::tuple<int, int, double>, std::vector<DVec3>> cache;
    std::vector<DVec3>& grid = cache[{Q, V, cfg.quadSize}];
    const double R = ctx.Kerbin().Radius;
    if (grid.empty()) {
        grid.resize(static_cast<size_t>(G) * G);
        for (int z = 0; z < G; z++)
            for (int x = 0; x < G; x++) {
                const DVec3 p = padPoint + f.east * (-half + x * step) + f.north * (-half + z * step);
                const DVec3 dir = p.Normalized();
                const double r = ctx.SurfaceRadius(ksp::Vector3d(dir.GetX(), dir.GetY(), dir.GetZ()));
                grid[static_cast<size_t>(z) * G + x] = dir * r;
            }
    }
    out.minHeight = 1e30;
    out.maxHeight = -1e30;
    for (const DVec3& g : grid) {
        out.minHeight = std::min(out.minHeight, g.Length() - R);
        out.maxHeight = std::max(out.maxHeight, g.Length() - R);
    }
    auto t1 = Clock::now();
    // ---- one static MeshShape body per quad
    BodyInterface& bi = world.Bodies();
    for (int qz = 0; qz < Q; qz++)
        for (int qx = 0; qx < Q; qx++) {
            const int cx = qx * (V - 1) + (V - 1) / 2, cz = qz * (V - 1) + (V - 1) / 2;
            const DVec3 centre = grid[static_cast<size_t>(cz) * G + cx];
            TriangleList tris;
            tris.reserve(static_cast<size_t>(2 * (V - 1) * (V - 1)));
            auto vtx = [&](int gx, int gz) {
                const DVec3 d = grid[static_cast<size_t>(gz) * G + gx] - centre;
                return Float3(static_cast<float>(d.GetX()), static_cast<float>(d.GetY()), static_cast<float>(d.GetZ()));
            };
            for (int z = 0; z < V - 1; z++)
                for (int x = 0; x < V - 1; x++) {
                    const int gx = qx * (V - 1) + x, gz = qz * (V - 1) + z;
                    const Float3 a = vtx(gx, gz), b = vtx(gx + 1, gz), c = vtx(gx + 1, gz + 1), d = vtx(gx, gz + 1);
                    // winding: normals pointing away from the body centre (up)
                    tris.push_back(Triangle(a, c, b));
                    tris.push_back(Triangle(a, d, c));
                }
            out.triangles += static_cast<int>(tris.size());
            MeshShapeSettings ms(tris);
            auto res = ms.Create();
            if (res.HasError()) throw std::runtime_error(res.GetError().c_str());
            const DVec3 pos = centre - frameOrigin;
            BodyCreationSettings b(res.Get(), RVec3(static_cast<Real>(pos.GetX()), static_cast<Real>(pos.GetY()), static_cast<Real>(pos.GetZ())),
                                   Quat::sIdentity(), EMotionType::Static, Layers::STATIC);
            b.mFriction = 0.6f;
            out.bodies.push_back(bi.CreateAndAddBody(b, EActivation::DontActivate));
        }
    world.System().OptimizeBroadPhase();
    auto t2 = Clock::now();
    out.heightEvalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    out.buildMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    return out;
}

void ShiftStatics(World& world, const TerrainPatch& patch, Vec3Arg delta) {
    BodyInterface& bi = world.Bodies();
    for (const BodyID& id : patch.bodies) bi.SetPosition(id, bi.GetPosition(id) + delta, EActivation::DontActivate);
}

}  // namespace spike
