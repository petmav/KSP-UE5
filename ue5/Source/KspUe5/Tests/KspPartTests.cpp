// Headless automation tests of part rendering data (WP-UE-10). Local only: they read the game copy decompiled/KSP-dev. Run:
//   Scripts\test.ps1 -Filter KspUe5.Parts
#include "CoreMinimal.h"
#include "KspModel.h"
#include "KspPartAssets.h"
#include "KspSim.h"
#include "KspSimSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

// Every part of the stock game: its rig, its .mu files converted, its meshes built, its materials and textures made. The slice
// criterion of ue5/DESIGN.md 6.3 ("unknown shaders / missing textures = 0") is checked here over all parts, not only the slice's craft.
//
// Winding check: the facade flips the triangle order (Unity front faces are clockwise). With the right order a triangle's geometric
// normal in Unreal's front-face convention, -cross(B - A, C - A), points the way its vertex normals do; with the wrong order it points
// the other way. Counted over every triangle of every stock mesh that has normals.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKspPartsStockSweepTest, "KspUe5.Parts.StockSweep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FKspPartsStockSweepTest::RunTest(const FString& Parameters)
{
	const FKspGameConfig Config = UKspSimSubsystem::DefaultGameConfig();
	if (!FPaths::DirectoryExists(FPaths::Combine(Config.GameRoot, TEXT("GameData"))))
	{
		AddError(FString::Printf(TEXT("no game copy at %s: this test is local only"), *Config.GameRoot));
		return false;
	}
	if (FKspGame::IsInitialised() && !(FKspGame::GetConfig() == Config) && !FKspGame::Shutdown())
	{
		AddError(TEXT("core is initialised with another configuration and a sim is running"));
		return false;
	}
	FString Error;
	if (!FKspGame::Init(Config, Error))
	{
		AddError(FString::Printf(TEXT("FKspGame::Init failed: %s"), *Error));
		return false;
	}
	const int32 PartCount = FKspGame::PartCount();
	TestTrue(TEXT("the stock game has more than 300 parts"), PartCount > 300);

	// ---- winding and conversion, straight from the facade ----
	TSet<FString> Files;
	int32 Rigs = 0, RigNodes = 0;
	TArray<FString> NoRig;
	for (int32 Id = 1; Id <= PartCount; ++Id)
	{
		FKspPartRig Rig;
		if (!FKspModels::GetPartRig(uint32(Id), Rig))
		{
			NoRig.Add(FKspGame::ModelName(uint32(Id)));
			continue;
		}
		++Rigs;
		RigNodes += Rig.Nodes.Num();
		for (const FKspRigFile& File : Rig.Files)
		{
			Files.Add(File.Path + TEXT("|") + File.ParentUrl);
		}
	}
	int64 Agree = 0, Disagree = 0, Flat = 0;
	int32 MeshesWithoutNormals = 0, MeshesWithoutTangents = 0, MeshesWithoutUv = 0, Meshes = 0;
	const double T0 = FPlatformTime::Seconds();
	for (const FString& Entry : Files)
	{
		FString Path, ParentUrl;
		Entry.Split(TEXT("|"), &Path, &ParentUrl);
		FKspModelData Model;
		if (!FKspModels::LoadModel(Path, ParentUrl, Model, Error))
		{
			AddError(FString::Printf(TEXT("%s: %s"), *Path, *Error));
			continue;
		}
		for (const FKspMeshData& Mesh : Model.Meshes)
		{
			++Meshes;
			MeshesWithoutTangents += Mesh.Tangents.IsEmpty() ? 1 : 0;
			MeshesWithoutUv += Mesh.UV0.IsEmpty() ? 1 : 0;
			if (Mesh.Normals.IsEmpty())
			{
				++MeshesWithoutNormals;
				continue;
			}
			for (const FKspMeshSection& Section : Mesh.Sections)
			{
				for (int32 I = 0; I + 2 < Section.Indices.Num(); I += 3)
				{
					const uint32 A = Section.Indices[I], B = Section.Indices[I + 1], C = Section.Indices[I + 2];
					const FVector3f Face = -FVector3f::CrossProduct(Mesh.Positions[B] - Mesh.Positions[A], Mesh.Positions[C] - Mesh.Positions[A]);
					const float Dot = FVector3f::DotProduct(Face, Mesh.Normals[A] + Mesh.Normals[B] + Mesh.Normals[C]);
					if (Face.SizeSquared() < 1e-12f || Dot == 0.0f)
					{
						++Flat;
					}
					else
					{
						++(Dot > 0.0f ? Agree : Disagree);
					}
				}
			}
		}
	}
	const double LoadSeconds = FPlatformTime::Seconds() - T0;
	const double AgreeShare = double(Agree) / double(FMath::Max<int64>(1, Agree + Disagree));
	AddInfo(FString::Printf(TEXT("%d parts: %d with a rig (%d rig nodes), %d without (%s)"), PartCount, Rigs, RigNodes, NoRig.Num(), *FString::Join(NoRig, TEXT(", "))));
	AddInfo(FString::Printf(TEXT("%d model files converted in %.2f s: %d meshes (%d without normals, %d without tangents, %d without uv)"), Files.Num(), LoadSeconds, Meshes,
		MeshesWithoutNormals, MeshesWithoutTangents, MeshesWithoutUv));
	AddInfo(FString::Printf(TEXT("winding: %lld triangles face the way their vertex normals do, %lld the other way, %lld without area or without a verdict (%.4f %% agree)"),
		Agree, Disagree, Flat, 100.0 * AgreeShare));
	TestTrue(TEXT("triangles face the way their normals do (the winding flip is right)"), AgreeShare > 0.99);

	// ---- the shell's assets for every part ----
	UKspPartAssets* Assets = NewObject<UKspPartAssets>(GetTransientPackage());
	Assets->AddToRoot();
	int32 KeptNodes = 0, MeshNodes = 0, PartsThatDraw = 0;
	for (int32 Id = 1; Id <= PartCount; ++Id)
	{
		const FKspPartTemplate& Template = Assets->GetTemplate(uint32(Id));
		bool bDraws = false;
		for (const FKspPartNodeTemplate& Node : Template.Nodes)
		{
			KeptNodes += Node.bKeep ? 1 : 0;
			MeshNodes += Node.Mesh ? 1 : 0;
			bDraws |= Node.Mesh != nullptr;
		}
		PartsThatDraw += bDraws ? 1 : 0;
	}
	const UKspPartAssets::FStats& Stats = Assets->GetStats();
	AddInfo(FString::Printf(TEXT("part assets: %s"), *Assets->DescribeStats()));
	AddInfo(FString::Printf(TEXT("%d parts draw something; %d scene components per full set of parts, %d of them with a mesh"), PartsThatDraw, KeptNodes, MeshNodes));
	TestEqual(TEXT("materials with an unknown shader"), Stats.UnknownShaders.Num(), 0);
	TestEqual(TEXT("textures the game database does not have"), Stats.MissingTextures, 0);
	TestEqual(TEXT("textures that could not be used"), Stats.UnusableTextures, 0);
	TestEqual(TEXT("model files that failed to load"), Stats.ModelFilesFailed, 0);
	TestEqual(TEXT("parts without a model are the parts without a rig"), Stats.PartsWithoutModel, NoRig.Num());

	Assets->RemoveFromRoot();
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	TestTrue(TEXT("core shuts down"), FKspGame::Shutdown());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
