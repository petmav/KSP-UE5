// IAnimatedModule.cs: the modules ModuleAnimationGroup enables / disables with its deploy state (BaseConverter, ModuleResourceScanner,
// ModuleOrbitalScanner, ModuleOrbitalSurveyor; ModuleGPS (WP-30: misc_modules.hpp); ModuleGroundPart is not ported yet).
#pragma once

namespace ksp {

class IAnimatedModule {
public:
    virtual ~IAnimatedModule() = default;
    virtual void EnableModule() = 0;       // IAnimatedModule.cs:3
    virtual void DisableModule() = 0;      // :5
    virtual bool ModuleIsActive() = 0;     // :7
    virtual bool IsSituationValid() = 0;   // :9
};

}  // namespace ksp
