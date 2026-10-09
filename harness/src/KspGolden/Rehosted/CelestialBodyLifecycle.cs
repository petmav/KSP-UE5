using System;
using KspGolden.Infra;

namespace KspGolden.Rehosted
{
    /// <summary>
    /// RE-HOSTED (copied expression-for-expression, not called): the numeric statements of CelestialBody.Awake,
    /// CelestialBody.Start and CelestialBody.CBUpdate. These Unity callbacks begin with Transform access
    /// (base.transform, an InternalCall), so they cannot be invoked outside Unity. Every other call they make
    /// (SetupConstants, Orbit.Init/UpdateFromUT) is invoked directly on KSP's code.
    /// Source: decompiled/KSP-Decompiled/Assembly-CSharp/CelestialBody.cs (line numbers of the 1.12.5 decompile).
    /// </summary>
    public static class CelestialBodyLifecycle
    {
        /// <summary>CelestialBody.cs:456-475 (Awake)</summary>
        public static void Awake(CelestialBody cb)
        {
            cb.gMagnitudeAtCenter = cb.GeeASL * PhysicsGlobals.GravitationalAcceleration * Math.Pow(cb.Radius, 2.0);
            cb.Mass = cb.Radius * cb.Radius * (cb.GeeASL * PhysicsGlobals.GravitationalAcceleration) / 6.67408E-11;
            cb.gravParameter = cb.Mass * 6.67408E-11;
        }

        /// <summary>CelestialBody.cs:477-530 (Start). SetupConstants() is the real KSP method.</summary>
        public static void Start(CelestialBody cb)
        {
            if ((bool)cb.orbitDriver)
            {
                var orbit = cb.orbit;
                cb.sphereOfInfluence = orbit.semiMajorAxis * Math.Pow(cb.Mass / orbit.referenceBody.Mass, 0.4);
                cb.hillSphere = orbit.semiMajorAxis * (1.0 - orbit.eccentricity) * Math.Pow(cb.Mass / orbit.referenceBody.Mass, 1.0 / 3.0);
                if (cb.solarRotationPeriod)
                {
                    double num = cb.rotationPeriod;
                    double num2 = Math.PI * 2.0 * Math.Sqrt(Math.Pow(Math.Abs(orbit.semiMajorAxis), 3.0) / orbit.referenceBody.gravParameter);
                    double time = num * num2 / (num2 + num);
                    cb.rotationPeriod = time;
                }
            }
            else
            {
                cb.sphereOfInfluence = double.PositiveInfinity;
                cb.hillSphere = double.PositiveInfinity;
            }
            SetupConstantsDirect(cb);
            cb.oceanFogPQSDepthRecip = (float)(1.0 / cb.oceanFogPQSDepth);
        }

        public static string SetupConstantsNote;

        /// <summary>
        /// Calls the real CelestialBody.SetupConstants(). Its last statement,
        /// minOrbitalDistance = CelestialUtilities.GetMinimumOrbitalDistance(this, 1f), needs PQS terrain / TimeWarp /
        /// ContractDefs (contract-system only). If (and only if) the exception originates there, every earlier
        /// statement has already executed and the computed fields are kept; anything else is rethrown.
        /// </summary>
        static void SetupConstantsDirect(CelestialBody cb)
        {
            try { cb.SetupConstants(); }
            catch (Exception e)
            {
                string st = e.StackTrace ?? "";
                if (!st.Contains("GetMinimumOrbitalDistance")) throw;
                SetupConstantsNote = "SetupConstants: final statement (minOrbitalDistance via CelestialUtilities.GetMinimumOrbitalDistance) not evaluated outside Unity (" + e.GetType().Name + "); all preceding statements executed.";
            }
        }

        /// <summary>PhysicsGlobals.CalculateValues (PhysicsGlobals.cs:2717-2734), re-hosted: it reads FlightGlobals.</summary>
        public static void PhysicsGlobalsCalculateValues(PhysicsGlobals pg, System.Collections.Generic.List<CelestialBody> flightGlobalsBodies)
        {
            // FlightGlobals.GetHomeBody(): first body (in FlightGlobals.Bodies order) with isHomeWorld
            CelestialBody celestialBody = flightGlobalsBodies.Find(b => b.isHomeWorld);
            CelestialBody celestialBody2 = Planetarium.fetch.Sun;
            if (celestialBody != null)
            {
                while (celestialBody.referenceBody != celestialBody2 && celestialBody.referenceBody != null)
                {
                    celestialBody = celestialBody.referenceBody;
                }
                double solarLuminosity = Math.Pow(celestialBody.orbit.semiMajorAxis, 2.0) * 4.0 * Math.PI * PhysicsGlobals.SolarLuminosityAtHome;
                U.Set(pg, "solarLuminosity", solarLuminosity);
            }
        }

        /// <summary>Planetarium.UpdateCBsRecursive(FindRootBody(Sun)) at UT (Planetarium.cs:250-283).</summary>
        public static void UpdateAll(double ut)
        {
            var root = FindRootBody(Planetarium.fetch.Sun);
            UpdateRecursive(root, ut);
        }

        static CelestialBody FindRootBody(CelestialBody cb)
        {
            foreach (var c in cb.orbitingBodies)
                if ((bool)c.orbitDriver && c.orbitDriver.reverse) return FindRootBody(c);
            return cb;
        }

        static void UpdateRecursive(CelestialBody cb, double ut)
        {
            CBUpdate(cb, ut);
            foreach (var c in cb.orbitingBodies)
                if (!c.orbitDriver || !c.orbitDriver.reverse) UpdateRecursive(c, ut);
            if ((bool)cb.orbitDriver && cb.orbitDriver.reverse) UpdateRecursive(cb.referenceBody, ut);
        }

        /// <summary>CelestialBody.cs:532-590 (CBUpdate), numeric statements only. The transform/rotation writes
        /// (transformRight/Up, BodyFrame, bodyTransform.rotation, Planetarium inverse rotation) are frame
        /// bookkeeping for rendering and are omitted; orbitDriver.UpdateOrbit() is reduced to its UPDATE-mode
        /// numeric core orbit.UpdateFromUT(UT) (OrbitDriver.updateFromParameters).</summary>
        public static void CBUpdate(CelestialBody cb, double ut)
        {
            cb.gMagnitudeAtCenter = cb.GeeASL * PhysicsGlobals.GravitationalAcceleration * cb.Radius * cb.Radius;
            cb.Mass = cb.Radius * cb.Radius * (cb.GeeASL * PhysicsGlobals.GravitationalAcceleration) / 6.67408E-11;
            cb.gravParameter = cb.Mass * 6.67408E-11;
            if (cb.rotates && cb.rotationPeriod != 0.0 && (!cb.tidallyLocked || (cb.orbit != null && cb.orbit.period != 0.0)))
            {
                if (cb.tidallyLocked)
                {
                    cb.rotationPeriod = cb.orbit.period;
                }
                cb.rotPeriodRecip = 1.0 / cb.rotationPeriod;
                cb.angularVelocity = Vector3d.down * (Math.PI * 2.0 * cb.rotPeriodRecip);
                cb.zUpAngularVelocity = Vector3d.back * (Math.PI * 2.0 * cb.rotPeriodRecip);
                cb.rotationAngle = (cb.initialRotation + 360.0 * cb.rotPeriodRecip * ut) % 360.0;
                cb.angularV = cb.angularVelocity.magnitude;
                if (!cb.inverseRotation)
                {
                    cb.directRotAngle = (cb.rotationAngle - Planetarium.InverseRotAngle) % 360.0;
                    Planetarium.CelestialFrame.PlanetaryFrame(0.0, 90.0, cb.directRotAngle, ref cb.BodyFrame);
                }
            }
            if ((bool)cb.orbitDriver)
            {
                cb.orbitDriver.orbit.UpdateFromUT(ut);
            }
            CelestialBody celestialBody = cb;
            CelestialBody sun = Planetarium.fetch.Sun; // (Planetarium.fetch != null) ? fetch.Sun : FlightGlobals.Bodies[0]
            while (celestialBody.referenceBody != sun && celestialBody.referenceBody != null)
            {
                celestialBody = celestialBody.referenceBody;
            }
            if (celestialBody.orbit != null)
            {
                double num = celestialBody.orbit.period - cb.rotationPeriod;
                cb.solarDayLength = (num == 0.0) ? double.MaxValue : celestialBody.orbit.period * cb.rotationPeriod / num;
            }
            else
            {
                cb.solarDayLength = 1.0;
            }
        }
    }
}
