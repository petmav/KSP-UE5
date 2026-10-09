// FlightIntegrator.cs:808-985 (atmosphere part), operation for operation. All double maths; the only float is the
// native FloatCurve evaluation of DragCurvePseudoReynolds.
#include "ksp/celestial/flight_atmosphere.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/physics/physics_globals.hpp"

namespace ksp {

namespace M = dotnet::Math;

namespace {
// System.Math.Max(double, double): NaN-propagating
double CsMax(double val1, double val2) {
    if (val1 > val2) return val1;
    if (val1 != val1) return val1;
    return val2;
}
}  // namespace

// FlightIntegrator.cs:808
void FlightIntegratorAtmosphere::CalculatePressure() {
    if (currentMainBody->atmosphere && altitude <= currentMainBody->atmosphereDepth) {
        staticPressurekPa = currentMainBody->GetPressure(altitude);
        staticPressureAtm = staticPressurekPa * 0.009869232667160128;
    } else {
        staticPressurekPa = 0.0;
        staticPressureAtm = 0.0;
    }
}

// FlightIntegrator.cs:861
double FlightIntegratorAtmosphere::CalculateDensityThermalLerp() const {
    double num = density;
    if (mach > 1.0) {
        const double num2 = mach * mach;
        num = (currentMainBody->atmosphereAdiabaticIndex + 1.0) * num2 /
              (2.0 + (currentMainBody->atmosphereAdiabaticIndex - 1.0) * num2) * num;
    }
    if (num < 0.0625) return 1.0 - M::Sqrt(M::Sqrt(num));
    if (num < 0.25) return 0.75 - M::Sqrt(num);
    return 0.0625 / num;
}

// FlightIntegrator.cs:880
double FlightIntegratorAtmosphere::CalculateBackgroundRadiationTemperature(double ambientTemp) const {
    return UtilMath::Lerp(ambientTemp, PhysicsGlobals::SpaceTemperature(), densityThermalLerp);
}

// FlightIntegrator.cs:885
void FlightIntegratorAtmosphere::CalculateConstantsVacuum() {
    atmosphericTemperature = PhysicsGlobals::SpaceTemperature();
    externalTemperature = PhysicsGlobals::SpaceTemperature();
    solarFluxMultiplier = 1.0;
    convectiveCoefficient = 0.0;
    convectiveMachFlux = 0.0;
    density = 0.0;
    dynamicPressurekPa = 0.0;
    mach = 0.0;
    speedOfSound = 0.0;
    pseudoReynolds = 0.0;
    pseudoReLerpTimeMult = 1.0;
    pseudoReDragMult = 1.0;
}

// FlightIntegrator.cs:917
void FlightIntegratorAtmosphere::CalculateConstantsAtmosphere() {
    currentMainBody->GetSolarAtmosphericEffects(sunDot, density, solarAirMass, solarFluxMultiplier);
    solarFlux *= solarFluxMultiplier;
    atmosphericTemperature = currentMainBody->GetFullTemperature(altitude, atmosphereTemperatureOffset);
    density = CalculateAtmosphericDensity(staticPressurekPa, atmosphericTemperature);
    dynamicPressurekPa = 0.0005 * density * spd * spd;
    speedOfSound = currentMainBody->GetSpeedOfSound(staticPressurekPa, density);
    if (speedOfSound > 0.0)
        mach = spd / speedOfSound;
    else
        mach = 0.0;
    convectiveMachLerp = M::Pow(UtilMath::Clamp01((mach - PhysicsGlobals::NewtonianMachTempLerpStartMach()) /
                                                  (PhysicsGlobals::NewtonianMachTempLerpEndMach() -
                                                   PhysicsGlobals::NewtonianMachTempLerpStartMach())),
                                PhysicsGlobals::NewtonianMachTempLerpExponent());
    externalTemperature = CsMax(atmosphericTemperature, CalculateShockTemperature());
    convectiveCoefficient = CalculateConvectiveCoefficient();
    pseudoReynolds = density * spd;
    pseudoReLerpTimeMult = 1.0 / (PhysicsGlobals::TurbulentConvectionEnd() - PhysicsGlobals::TurbulentConvectionStart());
    pseudoReDragMult = PhysicsGlobals::DragCurvePseudoReynolds().Evaluate(static_cast<float>(pseudoReynolds));
}

// FlightIntegrator.cs:944
double FlightIntegratorAtmosphere::CalculateShockTemperature() const {
    double num = spd * PhysicsGlobals::NewtonianTemperatureFactor();
    if (convectiveMachLerp > 0.0) {
        const double b = PhysicsGlobals::MachTemperatureScalar() * M::Pow(spd, PhysicsGlobals::MachTemperatureVelocityExponent());
        num = UtilMath::LerpUnclamped(num, b, convectiveMachLerp);
    }
    return num * static_cast<double>(reentryHeatScale) * currentMainBody->shockTemperatureMultiplier;
}

// FlightIntegrator.cs:955
double FlightIntegratorAtmosphere::CalculateAtmosphericDensity(double pres, double temp) const {
    return currentMainBody->GetDensity(pres, temp);
}

// FlightIntegrator.cs:965
double FlightIntegratorAtmosphere::CalculateConvectiveCoefficient() const { return CalculateConvectiveCoefficient(situation); }

// FlightIntegrator.cs:970
double FlightIntegratorAtmosphere::CalculateConvectiveCoefficient(VesselSituations sit) const {
    double num;
    if (sit == VesselSituations::SPLASHED) {
        num = (PhysicsGlobals::ConvectionFactorSplashed() * PhysicsGlobals::NewtonianConvectionFactorBase() +
               M::Pow(spd, PhysicsGlobals::NewtonianVelocityExponent()) * 10.0) *
              PhysicsGlobals::NewtonianConvectionFactorTotal();
    } else if (convectiveMachLerp == 0.0) {
        num = CalculateConvectiveCoefficientNewtonian();
    } else if (convectiveMachLerp != 1.0) {
        num = UtilMath::LerpUnclamped(CalculateConvectiveCoefficientNewtonian(), CalculateConvectiveCoefficientMach(),
                                      convectiveMachLerp);
    } else {
        num = CalculateConvectiveCoefficientMach();
    }
    return num * currentMainBody->convectionMultiplier;
}

// FlightIntegrator.cs:977
double FlightIntegratorAtmosphere::CalculateConvectiveCoefficientNewtonian() const {
    return ((density > 1.0) ? density : M::Pow(density, PhysicsGlobals::NewtonianDensityExponent())) *
           (PhysicsGlobals::NewtonianConvectionFactorBase() + M::Pow(spd, PhysicsGlobals::NewtonianVelocityExponent())) *
           PhysicsGlobals::NewtonianConvectionFactorTotal();
}

// FlightIntegrator.cs:982
double FlightIntegratorAtmosphere::CalculateConvectiveCoefficientMach() const {
    return 1E-07 * PhysicsGlobals::MachConvectionFactor() *
           ((density > 1.0) ? density : M::Pow(density, PhysicsGlobals::MachConvectionDensityExponent())) *
           M::Pow(spd, PhysicsGlobals::MachConvectionVelocityExponent());
}

}  // namespace ksp
