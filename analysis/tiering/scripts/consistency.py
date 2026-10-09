"""Consistency checks on tiering.csv (run after merge.py). Prints suspicious rows for lead review.

Run: python -I consistency.py
"""
import csv, json, os

BASE = r"C:/Projects/KSP-UE5/analysis/tiering"
WORK = os.path.join(BASE, "work")
ANCHORS = {
    "ConfigNode.cs": "A", "IConfigNode.cs": "A", "UrlDir.cs": "A", "FloatCurve.cs": "A", "DoubleCurve.cs": "A",
    "Vector3Curve.cs": "A", "Vector3d.cs": "A", "UnityEngine/QuaternionD.cs": "A", "Matrix4x4D.cs": "A",
    "UtilMath.cs": "A", "Orbit.cs": "A", "PatchedConicSolver.cs": "A", "PatchedConics.cs": "A",
    "TransferMath.cs": "A", "Planetarium.cs": "A", "OrbitDriver.cs": "B", "TimeWarp.cs": "B", "Krakensbane.cs": "B",
    "FloatingOrigin.cs": "B", "CelestialBody.cs": "B", "Part.cs": "B", "Vessel.cs": "B", "PartModule.cs": "B",
    "PartJoint.cs": "B", "VesselModule.cs": "B", "ScenarioModule.cs": "B", "FlightIntegrator.cs": "B",
    "FlightGlobals.cs": "B", "FlightDriver.cs": "B", "HighLogic.cs": "B", "PQS.cs": "B", "PQ.cs": "B",
    "PQSMod.cs": "B", "EditorLogic.cs": "B", "ShipConstruct.cs": "A", "ShipConstruction.cs": "B",
    "ModuleEngines.cs": "B", "ModuleRCS.cs": "B", "ModuleGimbal.cs": "B", "ModuleWheelBase.cs": "B",
    "KerbalEVA.cs": "B", "ModuleDockingNode.cs": "B", "ModuleDecouple.cs": "B", "KSP/UI/Screens/StageManager.cs": "B",
    "UIPartActionWindow.cs": "C", "DeltaVStageInfo.cs": "A", "PartSet.cs": "A", "PartResource.cs": "A",
    "PartResourceList.cs": "A", "PartResourceDefinition.cs": "A", "ResourceFlowMode.cs": "A",
    "ResearchAndDevelopment.cs": "A", "ScienceSubject.cs": "A", "Funding.cs": "A", "Reputation.cs": "A",
    "Contracts/ContractSystem.cs": "A", "Contracts/Contract.cs": "A", "CommNet/CommNetwork.cs": "A",
    "KerbalRoster.cs": "A", "ProtoCrewMember.cs": "A", "Game.cs": "A", "GamePersistence.cs": "A",
    "ProtoVessel.cs": "B", "ProtoPartSnapshot.cs": "B", "ProtoPartModuleSnapshot.cs": "A",
    "ProtoPartResourceSnapshot.cs": "A", "GameDatabase.cs": "B", "PartLoader.cs": "B", "AssemblyLoader.cs": "C",
    "PhysicsGlobals.cs": "A", "DragCubeList.cs": "A", "DragCube.cs": "A", "DragCubeSystem.cs": "B",
    "GameSettings.cs": "C", "KSP/Localization/Localizer.cs": "C", "SteamManager.cs": "C", "AnalyticsUtil.cs": "C",
}
PHYS = ("Rigidbody", "Joint", "Collider", "Raycast/Query", "Physics.", "WheelCollider", "Collision/Force")
UIK = ("UnityEngine.UI", "TMPro", "IMGUI", "EventSystems")


def main():
    rows = {r["path"]: r for r in csv.DictReader(open(os.path.join(BASE, "tiering.csv"), encoding="utf-8"))}
    auto = json.load(open(os.path.join(WORK, "auto.json"), encoding="utf-8"))
    print("== anchor mismatches")
    for rel, t in ANCHORS.items():
        r = rows.get("Assembly-CSharp/" + rel)
        if r is None:
            print("  missing", rel)
        elif r["tier"] != t:
            print(f"  {rel}: is {r['tier']} ({r['review_source']}) expected {t}")
    feats = {json.loads(l)["path"]: json.loads(l) for l in open(os.path.join(WORK, "features.jsonl"), encoding="utf-8")}
    print("== A rows with notable physics/scene coupling (phys>=4 or GameObject+Transform>=30)")
    for p, r in rows.items():
        if r["tier"] != "A":
            continue
        api = feats[p]["api"]
        phys = sum(api[k] for k in PHYS)
        om = api["GameObject"] + api["Transform"]
        if phys >= 4 or om >= 30:
            print(f"  {p} L={r['lines']} phys={phys} om={om} [{r['review_source']}] {r['rationale'][:90]}")
    print("== C rows that are gameplay hosts (PartModule/VesselModule/ScenarioModule/Contract*) without UI refs")
    for p, r in rows.items():
        if r["tier"] == "C" and r["kind"] in ("PartModule", "VesselModule", "ScenarioModule", "Contract", "ContractParameter"):
            api = feats[p]["api"]
            if sum(api[k] for k in UIK) == 0:
                print(f"  {p} L={r['lines']} kind={r['kind']} [{r['review_source']}] {r['rationale'][:90]}")
    print("== B rows with no physics and little scene use (phys==0, om<=3, rend==0)")
    for p, r in rows.items():
        if r["tier"] != "B":
            continue
        api = feats[p]["api"]
        phys = sum(api[k] for k in PHYS)
        om = api["GameObject"] + api["Transform"] + api["GetComponent"] + api["Instantiate/Destroy"]
        rend = api["Renderer"] + api["Mesh"] + api["Material"] + api["Animation"]
        if phys == 0 and om <= 3 and rend == 0:
            print(f"  {p} L={r['lines']} kind={r['kind']} [{r['review_source']}] {r['rationale'][:90]}")


if __name__ == "__main__":
    main()
