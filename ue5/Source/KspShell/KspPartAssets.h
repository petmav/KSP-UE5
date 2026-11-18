#pragma once

#include "CoreMinimal.h"
#include "KspModel.h"
#include "UObject/Object.h"

#include "KspPartAssets.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UTexture2D;

/** One node of a part's model as the shell builds it: a scene component, with a static mesh when the node draws one. */
struct FKspPartNodeTemplate
{
	FName Name;
	/** Index of the parent node in the template, INDEX_NONE: attached to the part's root. */
	int32 Parent = INDEX_NONE;
	FTransform Local = FTransform::Identity;
	UStaticMesh* Mesh = nullptr;
	/** One per section of the mesh. */
	TArray<UMaterialInterface*> Materials;
	bool bCastShadows = true;
	/** False: neither the node nor anything below it draws; no component is made for it. */
	bool bKeep = false;
};

/** The component tree of one part model (index-aligned with the part's rig, FKspPartRig::Nodes). */
struct FKspPartTemplate
{
	TArray<FKspPartNodeTemplate> Nodes;
};

/**
 * Builds and keeps what the shell draws parts with (WP-UE-10, ue5/DESIGN.md 3.4), all from the game's own files at run time:
 *  - one UStaticMesh per mesh of a `.mu` file (built once per file, shared by every part instance);
 *  - one UTexture2D per texture url (DDS blocks uploaded as stored, MBM raw pixels, PNG / TGA / JPEG decoded);
 *  - one UMaterialInstanceDynamic per model material, on the master material the KSP shader name maps to;
 *  - one FKspPartTemplate per part (model id), the tree UKspSimSubsystem instantiates per part.
 * Counts of everything that could not be honoured are kept for the slice's "0 unknown shaders, 0 missing textures" criterion.
 */
UCLASS()
class KSPSHELL_API UKspPartAssets : public UObject
{
	GENERATED_BODY()

public:
	/** The template of a part; it has no nodes when the part has no readable model. The reference stays valid. */
	const FKspPartTemplate& GetTemplate(uint32 ModelId);
	/** The texture of a GameDatabase url as the model wants it; null when the database has none or the file cannot be used. */
	UTexture2D* GetTexture(const FString& Url, bool bAsNormalMap);

	struct FStats
	{
		int32 Templates = 0;
		int32 PartsWithoutModel = 0;
		int32 ModelFiles = 0;
		int32 ModelFilesFailed = 0;
		int32 Meshes = 0;
		int64 Vertices = 0;
		int64 Triangles = 0;
		int32 SkinnedRenderers = 0;
		int32 Materials = 0;
		int32 Textures = 0;
		int32 MissingTextures = 0;
		int32 UnusableTextures = 0;
		double BuildSeconds = 0.0;
		/** Shader names without a master material and how many materials use them. */
		TMap<FString, int32> UnknownShaders;
	};
	const FStats& GetStats() const { return Stats; }
	FString DescribeStats() const;

private:
	struct FModelEntry
	{
		bool bLoaded = false;
		FKspModelData Data;
		/** Per Data.Meshes entry; null when the mesh could not be built. */
		TArray<UStaticMesh*> Meshes;
		/** .mu node id -> index into Data.Meshes of the renderer on that node. */
		TMap<int32, int32> MeshOfNode;
	};

	FModelEntry& GetModel(const FKspRigFile& File);
	UStaticMesh* BuildStaticMesh(const FKspMeshData& Mesh);
	/** Null for a material that draws nothing (DepthMask). */
	UMaterialInterface* GetMaterial(const FKspRigFile& File, const FModelEntry& Model, int32 MaterialId);
	UMaterialInterface* GetMaster(const TCHAR* Name);

	TMap<uint32, TUniquePtr<FKspPartTemplate>> Templates;
	TMap<FString, TUniquePtr<FModelEntry>> Models;
	/** Key: file path, texture replacements, material id. */
	TMap<FString, UMaterialInterface*> MaterialOfKey;
	TMap<FString, UTexture2D*> TextureOfKey;
	FStats Stats;

	/** Everything created here, kept alive for the garbage collector (the maps above hold plain pointers). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Keep;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInterface>> Masters;
};
