using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Threading;
using KspGolden.Infra;
using KspGolden.Rehosted;
using UnityEngine;

namespace KspGolden
{
    /// <summary>
    /// Builds the minimal KSP world the generators need, in the same order Unity/KSP do it:
    /// culture (HighLogic.cs:125), PhysicsGlobals (level0 prefab + Physics.cfg via LoadDatabase),
    /// Planetarium (level9 + Awake), and the 17 CelestialBody/OrbitDriver objects from the PSystem prefab in
    /// sharedassets9 (ctor + serialized overlay, then Awake/Start/CBUpdate).
    /// </summary>
    public static class KspEnv
    {
        public static PhysicsGlobals Physics;
        public static Planetarium Planetarium;
        public static List<CelestialBody> FlightGlobalsBodies;
        public static readonly List<CelestialBody> BodiesBySpawn = new List<CelestialBody>();
        public static readonly Dictionary<string, CelestialBody> Body = new Dictionary<string, CelestialBody>();
        public static readonly Dictionary<CelestialBody, JObj> SerializedBody = new Dictionary<CelestialBody, JObj>();
        public static readonly List<string> Notes = new List<string>();
        public static string PhysicsCfgPath;

        public static void InitCulture()
        {
            // HighLogic.cs:125  Thread.CurrentThread.CurrentCulture = CultureInfo.CreateSpecificCulture("en");
            // KSP runs on Unity's Mono, whose culture tables (native, inside mono-2.0-bdwgc.dll) give en-US the symbols
            // "Infinity" / "-Infinity" / "NaN" (verified by scanning the shipped DLL's UTF-8 string table: no U+221E).
            // .NET Framework on current Windows takes en-US from the OS and uses U+221E. Patch the symbols so that
            // float.Parse / ToString behave like KSP's Mono.
            var c = (CultureInfo)CultureInfo.CreateSpecificCulture("en").Clone();
            c.NumberFormat.PositiveInfinitySymbol = "Infinity";
            c.NumberFormat.NegativeInfinitySymbol = "-Infinity";
            c.NumberFormat.NaNSymbol = "NaN";
            Thread.CurrentThread.CurrentCulture = c;
        }

        public static void InitLogging(bool echo)
        {
            Run.Log = new CaptureLogHandler { Echo = echo };
            Debug.unityLogger.logHandler = Run.Log;
        }

        public static void InitPhysicsGlobals()
        {
            var ser = (JObj)JsonReader.ParseFile(Path.Combine(Run.SerializedDir, "physicsglobals.serialized.json"));
            Physics = U.New<PhysicsGlobals>();
            var rep = SerializedOverlay.Apply(Physics, ser.Obj("PhysicsGlobals"));
            foreach (var u in rep.Unmapped.Where(u => !u.StartsWith("blackBodyRadiation.")))
                Notes.Add("PhysicsGlobals serialized field not found on type: " + u);
            U.SetStatic(typeof(PhysicsGlobals), "instance", Physics);
            // Awake (PhysicsGlobals.cs:2736-2750) minus Shader.PropertyToID (render-only):
            //   physicsDatabaseFilename = KSPUtil.ApplicationRootPath + "Physics.cfg"  -> harness copy of Physics.cfg
            //   SurfaceCurves = { DragCurveTail, dragCurveSurface, dragCurveMultiplier, dragCurveTip }
            PhysicsCfgPath = Path.Combine(Run.InputsDir, "Physics.cfg");
            U.Set(Physics, "physicsDatabaseFilename", PhysicsCfgPath);
            SetSurfaceCurves();
            // Start (PhysicsGlobals.cs:2752): if (!LoadDatabase()) SaveDatabase();  -- the real method
            if (!Physics.LoadDatabase()) throw new InvalidOperationException("PhysicsGlobals.LoadDatabase failed for " + PhysicsCfgPath);
            // LoadDatabase loads DRAG_* nodes *into* the existing FloatCurve objects (dragCurveTip.Load(node), :3382-3415),
            // so the SurfaceCurves struct captured in Awake sees the Physics.cfg curves, exactly as in game.
            // Start: bodyLiftCurve = GetLiftingSurfaceCurve("BodyLift")
            U.Set(Physics, "bodyLiftCurve", PhysicsGlobals.GetLiftingSurfaceCurve("BodyLift"));
        }

        static void SetSurfaceCurves()
        {
            var sc = default(PhysicsGlobals.SurfaceCurvesList);
            sc.dragCurveTail = PhysicsGlobals.DragCurveTail;
            sc.dragCurveSurface = U.Get<FloatCurve>(Physics, "dragCurveSurface");
            sc.dragCurveMultiplier = U.Get<FloatCurve>(Physics, "dragCurveMultiplier");
            sc.dragCurveTip = U.Get<FloatCurve>(Physics, "dragCurveTip");
            PhysicsGlobals.SurfaceCurves = sc;
        }

        public static PartResourceLibrary Resources;

        /// <summary>PartResourceLibrary.Awake + LoadDefinitions, with GameDatabase replaced by a scan of the copied stock
        /// cfgs for RESOURCE_DEFINITION nodes (PartResourceLibrary.cs:22-62). resourceDefinitions.Add(ConfigNode) is real.</summary>
        public static void InitResources()
        {
            if (Resources != null) return;
            Resources = U.New<PartResourceLibrary>();
            typeof(PartResourceLibrary).GetProperty("Instance").GetSetMethod(true).Invoke(null, new object[] { Resources });
            Resources.resourceDefinitions = new PartResourceDefinitionList();
            string root = Path.Combine(Run.InputsDir, "GameData");
            foreach (var file in Directory.GetFiles(root, "*.cfg", SearchOption.AllDirectories).OrderBy(f => f, StringComparer.Ordinal))
            {
                var cn = ConfigNode.Load(file);
                if (cn == null) continue;
                foreach (var n in cn.GetNodes("RESOURCE_DEFINITION")) Resources.resourceDefinitions.Add(n);
            }
        }

        public static void InitPlanetarium()
        {
            var ser = (JObj)JsonReader.ParseFile(Path.Combine(Run.SerializedDir, "scene_singletons.serialized.json"));
            Planetarium = U.New<Planetarium>();
            SerializedOverlay.Apply(Planetarium, ser.Obj("Planetarium"));
            Planetarium.orbits = new List<OrbitDriver>();
            // Planetarium.Awake (Planetarium.cs:208-217) is pure: fetch = this; Zup = PlanetaryFrame(0,90,inverseRotAngle); rotation = ...
            U.SetStatic(typeof(Planetarium), "fetch", null);
            U.Call(Planetarium, "Awake");
            // FlightGlobals is NOT hosted: its static initializer calls Quaternion.Euler (a Unity InternalCall,
            // FlightGlobals.cs:100), so the type cannot be initialised outside Unity. Code that needs it is re-hosted.
        }

        public static void InitBodies(double ut = 0.0)
        {
            var root = (JObj)JsonReader.ParseFile(Path.Combine(Run.SerializedDir, "celestial_bodies.serialized.json"));
            var entries = root.Arr("bodies").Cast<JObj>().ToList();
            // PSystemManager.SpawnBody: Instantiate (=> Awake), wire parent/orbit, depth-first over the PSystem tree.
            foreach (var e in entries)
            {
                var cbData = e.Obj("celestialBody");
                var cb = U.New<CelestialBody>();
                var rep = SerializedOverlay.Apply(cb, cbData);
                foreach (var u in rep.Unmapped) Notes.Add("CelestialBody serialized field not found: " + u);
                cb.orbitingBodies = new List<CelestialBody>();
                cb.flightGlobalsIndex = (int)SerializedOverlay.Num(e["flightGlobalsIndex"]);
                string parent = e.Str("parent");
                if (e["orbitDriver"] is JObj odData)
                {
                    var od = U.New<OrbitDriver>();
                    var orep = SerializedOverlay.Apply(od, odData);
                    foreach (var u in orep.Unmapped) Notes.Add("OrbitDriver serialized field not found: " + u);
                    // OrbitDriver.Awake: celestialBody = GetComponent<CelestialBody>(); CelestialBody.Awake: orbitDriver = GetComponent<OrbitDriver>()
                    od.celestialBody = cb;
                    cb.orbitDriver = od;
                    // SpawnBody: component.orbit.referenceBody = parent
                    od.orbit.referenceBody = Body[parent];
                }
                else cb.orbitDriver = null;
                if (parent == null) Planetarium.Sun = cb; // SpawnBody: Planetarium.fetch.Sun = celestialBody
                else Body[parent].orbitingBodies.Add(cb);
                if (cb.isHomeWorld) Planetarium.Home = cb;
                BodiesBySpawn.Add(cb);
                Body[cb.bodyName] = cb;
                SerializedBody[cb] = cbData;
                CelestialBodyLifecycle.Awake(cb);
            }
            // PSystemManager.PostSetup: FlightGlobals.fetch.bodies sorted by flightGlobalsIndex
            FlightGlobalsBodies = BodiesBySpawn.OrderBy(b => b.flightGlobalsIndex).ToList();
            // Start(): CelestialBody.Start then OrbitDriver.Start (orbit.Init(); updateFromParameters())
            foreach (var cb in BodiesBySpawn) CelestialBodyLifecycle.Start(cb);
            foreach (var cb in BodiesBySpawn)
            {
                if (cb.orbitDriver == null) continue;
                cb.orbitDriver.orbit.Init();              // OrbitDriver.Start (UPDATE mode)
                cb.orbitDriver.orbit.UpdateFromUT(ut);    // updateFromParameters -> orbit.UpdateFromUT(Planetarium UT)
                Planetarium.orbits.Add(cb.orbitDriver);   // OrbitDriver.Start: Planetarium.Orbits.Add(this)
            }
            Planetarium.time = ut;
            // first Planetarium.FixedUpdate: UpdateCBsRecursive(FindRootBody(Sun)) -> CBUpdate on every body
            CelestialBodyLifecycle.UpdateAll(ut);
            // PhysicsGlobals.OnFlightGlobalsReady -> CalculateValues (solarLuminosity); re-hosted because it reads FlightGlobals.
            CelestialBodyLifecycle.PhysicsGlobalsCalculateValues(Physics, FlightGlobalsBodies);
        }
    }
}
