// Headless automation test of the terrain view (WP-UE-11). Uses the repository's GameData mirror and core/data/terrain. Run:
//   Scripts\test.ps1 -Filter KspUe5.Terrain
#include "CoreMinimal.h"
#include "KspSim.h"
#include "KspSimSubsystem.h"
#include "KspTerrainView.h"
#include "KspTestPaths.h"
#include "KspTestWorld.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

// PhysProf-M2 on the KSC LaunchPad: the quads of Kerbin's surface that core sends must end up, in Unreal's world, under the vessel.
// The vessel comes through the vessel / part path of the snapshot (positions relative to the dominant body in core's world axes), the
// terrain through the terrain stream (floats relative to a quad origin in the body's fixed frame, placed by the body's rotation):
// two independent chains of conversion that only meet if the axis map, the body frame, the quad origins and the render anchor are right.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspTerrainPadQuadsTest, "KspUe5.Terrain.PadQuads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspTerrainPadQuadsTest::RunTest(const FString& Parameters)
{
	const FString Repo = KspTestPaths::RepoRoot();
	const FKspGameConfig Config = KspTestPaths::MirrorGameConfig(TEXT("terrain"));
	const FString Craft = FPaths::Combine(Repo, TEXT("analysis"), TEXT("physics"), TEXT("craft"), TEXT("PhysProf-M2.craft"));
	if (FKspGame::IsInitialised() && !(FKspGame::GetConfig() == Config) && !FKspGame::Shutdown())
	{
		AddError(TEXT("core is initialised with another configuration and a sim is running"));
		return false;
	}
	{
		FKspTestWorld TestWorld;
		UKspSimSubsystem* Subsystem = TestWorld.World->GetSubsystem<UKspSimSubsystem>();
		FString Error;
		if (!TestNotNull(TEXT("the game world has the sim subsystem"), Subsystem) ||
			!Subsystem->StartSandboxFlight(Config, TEXT("terrain"), Craft, TEXT("LaunchPad"), Error))
		{
			AddError(FString::Printf(TEXT("StartSandboxFlight failed: %s"), *Error));
			return false;
		}
		Subsystem->FixedFrameDt = 0.02;
		Subsystem->PresentOverride = true;   // build the scene although this process does not render
		for (int32 Frame = 0; Frame < 5 && Subsystem->GetSim(); ++Frame)
		{
			TestWorld.Tick(0.02f);
		}
		const FKspSim* Sim = Subsystem->GetSim();
		const UKspTerrainView* Terrain = Subsystem->GetTerrain();
		const FKspVesselState* Vessel = Sim ? Sim->ActiveVessel() : nullptr;
		if (!TestNotNull(TEXT("the sim runs"), Sim) || !TestNotNull(TEXT("the terrain view exists"), Terrain) || !TestNotNull(TEXT("M2 is the active vessel"), Vessel))
		{
			return false;
		}
		const UKspTerrainView::FStats& Stats = Terrain->GetStats();
		AddInfo(FString::Printf(TEXT("terrain: %s"), *Terrain->DescribeStats()));
		TestTrue(TEXT("Kerbin's surface around the pad is drawn"), Terrain->NumQuads() > 50);
		TestEqual(TEXT("quads drawn = sent - re-stitched - removed"), Terrain->NumQuads(), Stats.Added - Stats.Restitched - Stats.Removed);

		const FKspBodyState* Kerbin = Sim->Bodies().FindByPredicate([](const FKspBodyState& B) { return (B.Flags & KspBodyFlag::Dominant) != 0; });
		if (!TestNotNull(TEXT("the dominant body is in the snapshot"), Kerbin))
		{
			return false;
		}
		const FKspRenderAnchor& Anchor = Subsystem->GetAnchor();
		const FVector BodyCentre = Anchor.ToWorld(Kerbin->Position);
		const FVector VesselWorld = Anchor.ToWorld(Vessel->RootPosition);
		const FVector Up = (VesselWorld - BodyCentre).GetSafeNormal();
		const double RadiusCm = Kerbin->Radius * 100.0;

		// every vertex of every quad: its distance from the body's centre, and the one nearest to the vessel
		double MinRadius = TNumericLimits<double>::Max(), MaxRadius = 0.0, Nearest = TNumericLimits<double>::Max();
		FVector NearestVertex = FVector::ZeroVector, NearestNormal = FVector::ZeroVector;
		const UProceduralMeshComponent* NearestQuad = nullptr;
		int64 Vertices = 0;
		// the PQS vertex colours as they reach the mesh: sum and range per channel
		FVector4d ColourSum(0, 0, 0, 0);
		FColor ColourMin(255, 255, 255, 255), ColourMax(0, 0, 0, 0);
		for (const TPair<int32, TObjectPtr<UProceduralMeshComponent>>& Pair : Terrain->GetQuads())
		{
			const UProceduralMeshComponent* Component = Pair.Value;
			const FProcMeshSection* Section = Component ? const_cast<UProceduralMeshComponent*>(Component)->GetProcMeshSection(0) : nullptr;
			if (!Section)
			{
				continue;
			}
			const FTransform ToWorld = Component->GetComponentTransform();
			for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
			{
				const FVector World = ToWorld.TransformPosition(Vertex.Position);
				const double Radius = FVector::Dist(World, BodyCentre);
				MinRadius = FMath::Min(MinRadius, Radius);
				MaxRadius = FMath::Max(MaxRadius, Radius);
				const double Distance = FVector::Dist(World, VesselWorld);
				if (Distance < Nearest)
				{
					Nearest = Distance;
					NearestVertex = World;
					NearestNormal = ToWorld.TransformVectorNoScale(Vertex.Normal);
					NearestQuad = Component;
				}
				ColourSum += FVector4d(Vertex.Color.R, Vertex.Color.G, Vertex.Color.B, Vertex.Color.A);
				ColourMin = FColor(FMath::Min(ColourMin.R, Vertex.Color.R), FMath::Min(ColourMin.G, Vertex.Color.G), FMath::Min(ColourMin.B, Vertex.Color.B), FMath::Min(ColourMin.A, Vertex.Color.A));
				ColourMax = FColor(FMath::Max(ColourMax.R, Vertex.Color.R), FMath::Max(ColourMax.G, Vertex.Color.G), FMath::Max(ColourMax.B, Vertex.Color.B), FMath::Max(ColourMax.A, Vertex.Color.A));
				++Vertices;
			}
		}
		ColourSum /= double(FMath::Max<int64>(1, Vertices));
		AddInfo(FString::Printf(TEXT("vertex colours (0..255): mean r %.1f g %.1f b %.1f a %.1f; min %d %d %d %d; max %d %d %d %d"), ColourSum.X, ColourSum.Y, ColourSum.Z,
			ColourSum.W, ColourMin.R, ColourMin.G, ColourMin.B, ColourMin.A, ColourMax.R, ColourMax.G, ColourMax.B, ColourMax.A));
		// Kerbin around the KSC is grass, shore and sea: coloured, not black
		TestTrue(TEXT("the terrain has colours"), ColourSum.X + ColourSum.Y + ColourSum.Z > 30.0);
		const double Below = FVector::DotProduct(VesselWorld - NearestVertex, Up);
		AddInfo(FString::Printf(TEXT("%lld vertices; distance from Kerbin's centre %.1f to %.1f m beyond the radius; the vertex nearest to the vessel's root part is %.2f m away, ")
			TEXT("%.2f m below it; its normal . up = %.4f; vessel altitude %.2f m"),
			Vertices, (MinRadius - RadiusCm) / 100.0, (MaxRadius - RadiusCm) / 100.0, Nearest / 100.0, Below / 100.0, FVector::DotProduct(NearestNormal, Up), Vessel->Altitude));
		// the surface is within Kerbin's relief of the radius (ocean floor about -1.4 km, mountains below 7 km)
		TestTrue(TEXT("every terrain vertex is at Kerbin's surface"), MinRadius > RadiusCm - 2.0e5 && MaxRadius < RadiusCm + 8.0e5);
		// the pad is a structure on flat ground: the ground is a few tens of metres from the root part of a vessel standing on it, below it
		TestTrue(TEXT("the terrain is under the vessel"), Nearest < 100.0 * 100.0 && Below > 0.0);
		TestTrue(TEXT("the ground's normal points up"), FVector::DotProduct(NearestNormal, Up) > 0.9);

		// winding: the front face of a triangle (Unreal's convention, -cross(B - A, C - A)) of the quad under the vessel looks up
		if (NearestQuad)
		{
			const FProcMeshSection* Section = const_cast<UProceduralMeshComponent*>(NearestQuad)->GetProcMeshSection(0);
			const FTransform ToWorld = NearestQuad->GetComponentTransform();
			int32 FacingUp = 0, Triangles = 0;
			for (int32 I = 0; I + 2 < Section->ProcIndexBuffer.Num(); I += 3)
			{
				const FVector A = Section->ProcVertexBuffer[Section->ProcIndexBuffer[I]].Position;
				const FVector B = Section->ProcVertexBuffer[Section->ProcIndexBuffer[I + 1]].Position;
				const FVector C = Section->ProcVertexBuffer[Section->ProcIndexBuffer[I + 2]].Position;
				FacingUp += FVector::DotProduct(ToWorld.TransformVectorNoScale(-FVector::CrossProduct(B - A, C - A)), Up) > 0.0 ? 1 : 0;
				++Triangles;
			}
			AddInfo(FString::Printf(TEXT("the quad under the vessel: %d of %d triangles face up"), FacingUp, Triangles));
			TestEqual(TEXT("the triangles of the quad under the vessel face up"), FacingUp, Triangles);
		}
		Subsystem->StopSim();
	}
	TestTrue(TEXT("core shuts down once the sim is gone"), FKspGame::Shutdown());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
