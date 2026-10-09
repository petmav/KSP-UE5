// Headless automation tests. Run:
//   UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests KspUe5; Quit" -unattended -nopause -nullrhi -log
#include "CoreMinimal.h"
#include "KspCoreBridge.h"
#include "Misc/AutomationTest.h"
#include "Spike/SpikeMath.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspBridgeLinksCoreTest, "KspUe5.Bridge.LinksCore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspBridgeLinksCoreTest::RunTest(const FString& Parameters)
{
	const FString Info = FKspCoreBridge::GetCoreBuildInfo();
	AddInfo(FString::Printf(TEXT("kspcore build info: %s (stub=%d)"), *Info, FKspCoreBridge::IsStub() ? 1 : 0));
	TestFalse(TEXT("build info is non-empty"), Info.IsEmpty());
	TestTrue(TEXT("core built against the release CRT DLL (/MD)"), Info.Contains(TEXT("crt=MD ")));
	TestTrue(TEXT("core built with _ITERATOR_DEBUG_LEVEL=0"), Info.Contains(TEXT("idl=0")));
	TestEqual(TEXT("C ABI version"), FKspCoreBridge::GetCompiledAbiVersion(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspPrecisionTest, "KspUe5.Precision.Float32VsLwc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspPrecisionTest::RunTest(const FString& Parameters)
{
	using namespace KspSpike;
	// Camera 1 m above Kerbin at a generic site (all axes large), strafing 1 mm per frame; a static vertex 3 m away.
	const FVector3d Site = FVector3d(1, 1, 1).GetSafeNormal();
	const FVector3d East = FVector3d(-Site.Y, Site.X, 0).GetSafeNormal();
	const FVector3d North = (FVector3d(0, 0, 1) - Site * Site.Z).GetSafeNormal();
	const FQuat4d Rot = FRotationMatrix::MakeFromXZ(FVector(East), FVector(Site)).ToQuat();
	const FVector3d V = Site * (KerbinRadiusCm + 50.0) + East * 300.0 + North * 70.0;
	const FVector3d NearOrigin = Site * KerbinRadiusCm + East * 280.0; // primitive origin close to the vertex
	FJitterTracker F32, Rebased, Lwc, LwcPlanetCentred;
	for (int32 I = 0; I < 600; ++I)
	{
		const FVector3d C = Site * (KerbinRadiusCm + 100.0) + North * (0.1 * I);
		const FVector3d Exact = CamSpaceExact(V, C, Rot);
		F32.Add(CamSpaceF32World(V, C, FVector3d::ZeroVector, Rot), Exact);
		Rebased.Add(CamSpaceF32World(V, C, Site * KerbinRadiusCm, Rot), Exact);
		Lwc.Add(CamSpaceLwc(V, NearOrigin, C, Rot), Exact);
		LwcPlanetCentred.Add(CamSpaceLwc(V, FVector3d::ZeroVector, C, Rot), Exact);
	}
	AddInfo(FString::Printf(TEXT("max per-frame jitter (cm): f32 world no rebase %.4f | f32 rebased %.6f | LWC small-local %.6f | LWC planet-centred %.4f"),
		F32.MaxJitter, Rebased.MaxJitter, Lwc.MaxJitter, LwcPlanetCentred.MaxJitter));
	TestTrue(TEXT("float32 world space at 600 km jitters by >= 1 cm"), F32.MaxJitter >= 1.0);
	TestTrue(TEXT("float32 with the origin rebased near the camera stays < 0.01 cm"), Rebased.MaxJitter < 0.01);
	TestTrue(TEXT("LWC with small primitive-local coordinates stays < 0.01 cm"), Lwc.MaxJitter < 0.01);
	TestTrue(TEXT("LWC with planet-centred primitive coordinates jitters >= 1 cm"), LwcPlanetCentred.MaxJitter >= 1.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspCubeSphereTest, "KspUe5.Spike.CubeSpherePatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspCubeSphereTest::RunTest(const FString& Parameters)
{
	using namespace KspSpike;
	for (int32 Level : { 0, 7, 14 })
	{
		FPatchMesh Mesh;
		const int32 Cells = 1 << Level;
		BuildPatch(2, Level, Cells / 3, Cells / 2, 16, KerbinRadiusCm, false, false, EPatchColor::Terrain, FColor::White, Mesh);
		const int32 Grid = 17 * 17;
		double MaxRadiusErr = 0.0;
		double MaxLocal = 0.0;
		for (int32 I = 0; I < Grid; ++I)
		{
			const FVector3d P = Mesh.Origin + FVector3d(FVector3f(Mesh.Verts[I])); // as the GPU sees it (float local)
			MaxRadiusErr = FMath::Max(MaxRadiusErr, FMath::Abs(P.Length() - KerbinRadiusCm));
			MaxLocal = FMath::Max(MaxLocal, FVector3d(Mesh.Verts[I]).Length());
		}
		// Front faces: UE treats a triangle as front-facing when cross(B-A, C-A) points away from the viewer.
		int32 Outward = 0;
		const int32 GridTris = 16 * 16 * 2;
		for (int32 T = 0; T < GridTris; ++T)
		{
			const FVector3d A(Mesh.Verts[Mesh.Tris[T * 3]]), B(Mesh.Verts[Mesh.Tris[T * 3 + 1]]), C(Mesh.Verts[Mesh.Tris[T * 3 + 2]]);
			const FVector3d Radial = (Mesh.Origin + A).GetSafeNormal();
			Outward += FVector3d::DotProduct(FVector3d::CrossProduct(B - A, C - A), Radial) < 0.0 ? 1 : 0;
		}
		AddInfo(FString::Printf(TEXT("level %d: edge %.1f m, max local coord %.1f m, max radius error (float local) %.4f cm, front-facing outward %d/%d"),
			Level, PatchEdgeLength(Level, KerbinRadiusCm) / 100.0, MaxLocal / 100.0, MaxRadiusErr, Outward, GridTris));
		TestEqual(TEXT("all grid triangles face outward"), Outward, GridTris);
		if (Level == 14)
		{
			TestTrue(TEXT("leaf patch vertices are within 0.01 cm of the sphere"), MaxRadiusErr < 0.01);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
