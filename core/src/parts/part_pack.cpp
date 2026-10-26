// WP-20g: Part.Pack / Part.Unpack and the GameObject.SendMessage receivers of the part (see include/ksp/parts/part.hpp). C# paths are
// relative to decompiled/KSP-Decompiled/Assembly-CSharp/.
#include <string_view>

#include "ksp/events/game_events.hpp"
#include "ksp/modules/misc_modules.hpp"
#include "ksp/modules/module_cargo_part.hpp"
#include "ksp/modules/module_docking_node.hpp"
#include "ksp/modules/module_structural.hpp"
#include "ksp/modules/parachutes.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/serenity/base_servo.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

// GameObject.SendMessage(message, SendMessageOptions.DontRequireReceiver): the Part component has none of these methods; its modules
// follow in component order (Part.AddModule's AddComponent order = the module list). Unity calls private methods too, so every
// receiver of Assembly-CSharp counts whatever its access: ModuleCargoPart.cs:80 / :84 (virtual; ModuleGroundPart.cs:144 / :156
// override it), ModuleDockingNode.cs:681 / :689, BaseServo.cs:1177 / :1182, ModuleParachute.cs:811, LaunchClamp.cs:324 / :332 and
// the flag's FlagSite.cs:362 / :374. The empty PartJoint.cs:419 / :423 (a component of the part's GameObject) change nothing; the
// receiver of ModuleJointMotor.cs:170 is not ported (no stock part uses that module).
size_t Part::SendPartMessage(std::string_view message) {
    const bool pack = message == "OnPartPack", unpack = message == "OnPartUnpack";
    const bool vpack = message == "OnVesselPack", vunpack = message == "OnVesselUnpack";
    size_t n = 0;
    for (size_t i = 0; i < Modules().Count(); ++i) {
        PartModule* m = Modules().GetModule(i);
        if (m == nullptr) continue;
        if (pack || unpack) {
            if (auto* c = dynamic_cast<ModuleCargoPart*>(m)) {
                if (pack) c->OnPartPack();
                else c->OnPartUnpack();
                ++n;
            } else if (auto* d = dynamic_cast<ModuleDockingNode*>(m)) {
                if (pack) d->OnPartPack();
                else d->OnPartUnpack();
                ++n;
            } else if (auto* s = dynamic_cast<Expansions::Serenity::BaseServo*>(m)) {
                if (pack) s->OnPartPack();
                else s->OnPartUnpack();
                ++n;
            } else if (auto* p = dynamic_cast<ModuleParachute*>(m)) {
                if (unpack) {
                    p->OnPartUnpack();
                    ++n;
                }
            }
        } else if (vpack || vunpack) {
            if (auto* c = dynamic_cast<LaunchClamp*>(m)) {
                if (vpack) c->OnVesselPack();
                else c->OnVesselUnpack();
                ++n;
            } else if (auto* f = dynamic_cast<FlagSite*>(m)) {
                if (vpack) f->OnVesselPack();
                else f->OnVesselUnpack();
                ++n;
            }
        }
    }
    return n;
}

// Part.cs:3747
void Part::Pack() {
    if (fields.packed || State() == PartStates::DEAD) return;
    // :3753-3768 rb.isKinematic = true, PartBuoyancy disabled, terrainCollider destroyed, servoRb.isKinematic = true: the host's (the
    // reduced model drops the vessel's bodies; ContactHost runs PartBuoyancy for unpacked vessels only)
    if (Vessel::env.packPart) Vessel::env.packPart(*this);
    for (size_t i = 0, count = protoModuleCrew.size(); i < count; i++)  // :3769-3773
        if (protoModuleCrew[i]) protoModuleCrew[i]->OnPartPack(*this);
    onPack();                    // :3774
    SendPartMessage("OnPartPack");  // :3775
    GE::onPartPack.Fire(this);   // :3776
    // :3777-3780 attachJoint.SetUnbreakable(true, rigidAttachment): no joint exists while the bodies are dropped (reduced model)
    fields.packed = true;        // :3781
}

// Part.cs:3784
void Part::Unpack() {
    // :3786 ScheduleSetCollisionIgnores: the host's (the bodies are built with the vessel's collision filter)
    if (State() != PartStates::DEAD) {  // :3787 `rb && state != DEAD`
        // :3789-3797 rb / servoRb not kinematic, PartBuoyancy enabled: the host builds the vessel's bodies on its first part
        if (Vessel::env.unpackPart) Vessel::env.unpackPart(*this);
        onUnpack();  // :3798
    }
    for (size_t i = 0, count = protoModuleCrew.size(); i < count; i++)  // :3800-3804
        if (protoModuleCrew[i]) protoModuleCrew[i]->OnPartUnpack(*this);
    // :3805 ResetJoints (Part.cs:3811: attachJoint.SetUnbreakable(CheatOptions.UnbreakableJoints), CycleAutoStrut): the reduced model's
    // joints and autostruts are made with the bodies (WP-20c), breakable
    fields.packed = false;          // :3806
    SendPartMessage("OnPartUnpack");  // :3807
    GE::onPartUnpack.Fire(this);    // :3808
}

}  // namespace ksp
