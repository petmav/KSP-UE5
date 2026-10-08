// UnityEngine.Time: the values KSP's portable code reads (KerbalFSM, KFSMTimedEvent). Process-wide, like Unity's;
// the host's simulation loop advances them (kspcore never writes them). Not thread-safe.
#pragma once

namespace ksp {

struct Time {
    static inline float time = 0.0f;   // Time.time: seconds since startup at the start of this frame (float)
    static inline int frameCount = 0;  // Time.frameCount
};

}  // namespace ksp
