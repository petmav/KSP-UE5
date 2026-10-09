// UnityEngine.Time: the values KSP's portable code reads (KerbalFSM, KFSMTimedEvent). Process-wide, like Unity's;
// the host's simulation loop advances them (kspcore never writes them). Not thread-safe.
#pragma once

namespace ksp {

struct Time {
    static inline float time = 0.0f;   // Time.time: seconds since startup at the start of this frame (float)
    static inline int frameCount = 0;  // Time.frameCount
    // Time.timeScale / Time.fixedDeltaTime (WP-08: PatchedConicSolver.Update, OrbitDriver.RecalculateOrbit and
    // TimeWarp.fixedDeltaTime read them; KSP's TimeWarp.updateRate writes them, TimeWarp.cs:419). Unity defaults.
    static inline float timeScale = 1.0f;
    static inline float fixedDeltaTime = 0.02f;
};

}  // namespace ksp
