// WP-26: Part.AddShield / RemoveShield / OnShieldModified (Part.cs:7627-7680), the airstream shield bookkeeping that the
// cargo bay and fairing packages feed (IAirstreamShield implementers: ModuleCargoBay, ModuleDecouplerBase).
#include <algorithm>

#include "ksp/core/log.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/staging/staging_types.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

// Part.cs:7627 (`airstreamShields.AddUnique(shd); OnShieldModified(shd); return OnShieldModified;`)
void Part::AddShield(IAirstreamShield* shd) {
    if (std::find(airstreamShields.begin(), airstreamShields.end(), shd) == airstreamShields.end()) airstreamShields.push_back(shd);
    OnShieldModified(shd);
}

// Part.cs:7634
void Part::RemoveShield(IAirstreamShield* shd) {
    auto it = std::find(airstreamShields.begin(), airstreamShields.end(), shd);
    if (it != airstreamShields.end()) airstreamShields.erase(it);
    OnShieldModified(shd);
}

// Part.cs:7641
void Part::OnShieldModified(IAirstreamShield* /*shd*/) {
    for (size_t count = airstreamShields.size(); count-- > 0;) {
        IAirstreamShield* airstreamShield = airstreamShields[count];
        if (airstreamShield == nullptr) {
            log::Warning("[Part Airstream Shielding]: shield at i:" + std::to_string(count) + " was null and was removed.");
            airstreamShields.erase(airstreamShields.begin() + static_cast<std::ptrdiff_t>(count));
        }
        if (airstreamShield != nullptr && vessel != nullptr && airstreamShield->GetVessel() != nullptr &&
            airstreamShield->GetVessel()->persistentId != vessel->persistentId) {
            airstreamShields.erase(airstreamShields.begin() + static_cast<std::ptrdiff_t>(count));
        }
    }
    partIsShielded_ = !airstreamShields.empty();
    if (!partIsShielded_) return;
    partIsShielded_ = false;
    fields.isShieldedDisplay = Localizer::Format("#autoLOC_439840");
    size_t count2 = airstreamShields.size();
    for (;;) {
        if (count2-- == 0) return;
        if (airstreamShields[count2]->ClosedAndLocked()) break;
    }
    partIsShielded_ = true;
    fields.isShieldedDisplay = Localizer::Format("#autoLOC_439839");
}

}  // namespace ksp
