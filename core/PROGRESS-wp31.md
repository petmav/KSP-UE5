# PROGRESS WP-31 (branch wp31 from main 135fb5aa)
## Facts
- WP-04 already has FlightLog, Experience configs, ProtoCrewMember, KerbalRoster (crew.hpp/cpp). EXTEND, don't duplicate.
- dev copy (decompiled/KSP-dev) NOT readable from this worktree: corpus.saves is not registered in the worktree; name tables (TextAssets/CrewGenerator) unavailable -> CrewGenerator takes injected tables; only saves/physprofile/persistent.sfs exists (4 kerbals).
- Seam class VesselCrewManifest in save/ship_construct.hpp (WP-18) to be replaced by the real class (virtual methods kept).
## Plan
A experience.hpp/cpp (trait, effect, 20 effects) | B crew.cpp extend | C crew_generator | D crew manifests | E APSkill, ModuleExperienceManagement, KerbalMovementParameters | F tests+data gate | G docs
## Steps done
- [x] A-E written (uncompiled): include/ksp/kerbals/{experience,crew_generator,crew_manifest,kerbal_misc}.hpp, src/kerbals/*.cpp; crew.hpp/cpp extended (experienceTrait is now shared_ptr<ExperienceTrait>); seam VesselCrewManifest in ship_construct.hpp replaced by real class (crew_manifest.hpp)
- Next: build (run_tests.ps1 -BuildOnly), fix; tests/test_kerbals.cpp; data gate tool; docs
- [x] DONE: merged main d46be6e4; Windows fresh 33/33 (unit 375 cases / 708,505 assertions, WP-31 20 cases / 11,856), WSL linux-rel 33/33; STATUS "WP-31", README rows; check_cites 0 mismatches in src/kerbals
