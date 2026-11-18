#include "KspPartAssets.h"

#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "IImageWrapperModule.h"
#include "ImageCore.h"
#include "KspShell.h"
#include "KspSim.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "StaticMeshAttributes.h"
#include "TextureResource.h"

namespace
{
	/** What a KSP shader becomes: a master material (null name: the material draws nothing) and whether it has a specular term. */
	struct FKspShaderRule
	{
		const TCHAR* Shader;
		const TCHAR* Master;
		bool bSpecular;
	};

	// Every shader name the 551 stock .mu files use (core/STATUS.md WP-UE-05 "shader names (28 distinct, by use)").
	const FKspShaderRule ShaderRules[] = {
		{ TEXT("KSP/Diffuse"), TEXT("Opaque"), false },
		{ TEXT("KSP/Specular"), TEXT("Opaque"), true },
		{ TEXT("KSP/Bumped"), TEXT("Opaque"), false },
		{ TEXT("KSP/Bumped Specular"), TEXT("Opaque"), true },
		{ TEXT("KSP/Bumped Specular (Mapped)"), TEXT("Opaque"), true },
		{ TEXT("KSP/Emissive/Diffuse"), TEXT("Opaque"), false },
		{ TEXT("KSP/Emissive/Specular"), TEXT("Opaque"), true },
		{ TEXT("KSP/Emissive/Bumped Specular"), TEXT("Opaque"), true },
		{ TEXT("KSP/Emissive/Bumped Specular (Mapped)"), TEXT("Opaque"), true },
		{ TEXT("KSP/Lightwrapped/Specular"), TEXT("Opaque"), true },
		{ TEXT("KSP/InternalSpace"), TEXT("Opaque"), false },
		{ TEXT("Standard"), TEXT("Opaque"), true },
		{ TEXT("Legacy Shaders/Diffuse"), TEXT("Opaque"), false },
		{ TEXT("KSP/Alpha/Cutoff"), TEXT("Masked"), false },
		{ TEXT("KSP/Alpha/Cutoff Bumped"), TEXT("Masked"), false },
		{ TEXT("KSP/Alpha/CutoffBackground"), TEXT("Masked"), false },
		{ TEXT("KSP/Alpha/Translucent"), TEXT("Translucent"), false },
		{ TEXT("KSP/Alpha/Translucent Specular"), TEXT("Translucent"), true },
		{ TEXT("KSP/Specular (Transparent)"), TEXT("Translucent"), true },
		{ TEXT("KSP/Bumped Specular (Transparent)"), TEXT("Translucent"), true },
		{ TEXT("KSP/Alpha/Translucent Additive"), TEXT("Additive"), false },
		{ TEXT("KSP/Particles/Additive"), TEXT("Additive"), false },
		{ TEXT("KSP/Unlit"), TEXT("Unlit"), false },
		{ TEXT("KSP/UnlitColor"), TEXT("Unlit"), false },
		{ TEXT("KSP/Alpha/Unlit Transparent"), TEXT("UnlitTranslucent"), false },
		{ TEXT("KSP/Particles/Alpha Blended"), TEXT("UnlitTranslucent"), false },
		{ TEXT("Unlit/Transparent"), TEXT("UnlitTranslucent"), false },
		// writes depth only (hides what is behind it): nothing to draw
		{ TEXT("DepthMask"), nullptr, false },
	};

	const FKspShaderRule* FindShaderRule(const FString& Shader)
	{
		for (const FKspShaderRule& Rule : ShaderRules)
		{
			if (Shader == Rule.Shader)
			{
				return &Rule;
			}
		}
		return nullptr;
	}

	/** KSP renders in gamma space: a material colour multiplies the stored texture values. Unreal multiplies linear values. */
	FLinearColor GammaToLinear(const FLinearColor& C)
	{
		return FLinearColor(FMath::Pow(FMath::Max(C.R, 0.0f), 2.2f), FMath::Pow(FMath::Max(C.G, 0.0f), 2.2f), FMath::Pow(FMath::Max(C.B, 0.0f), 2.2f), C.A);
	}

	/** GameDatabase.BitmapToUnityNormalMap (GameDatabase.cs:616): (r, g, b, a) = (g, g, g, r). Pixels are BGRA8. */
	void ToUnityNormalMap(TArray64<uint8>& Bgra)
	{
		for (int64 I = 0; I + 3 < Bgra.Num(); I += 4)
		{
			const uint8 G = Bgra[I + 1], R = Bgra[I + 2];
			Bgra[I] = G;
			Bgra[I + 2] = G;
			Bgra[I + 3] = R;
		}
	}

	void FlipRows(TArray64<uint8>& Bgra, int32 Width, int32 Height)
	{
		const int64 Row = int64(Width) * 4;
		TArray<uint8> Temp;
		Temp.SetNumUninitialized(Row);
		for (int32 Y = 0; Y < Height / 2; ++Y)
		{
			uint8* A = Bgra.GetData() + Row * Y;
			uint8* B = Bgra.GetData() + Row * (Height - 1 - Y);
			FMemory::Memcpy(Temp.GetData(), A, Row);
			FMemory::Memcpy(A, B, Row);
			FMemory::Memcpy(B, Temp.GetData(), Row);
		}
	}

	UTexture2D* MakeTexture(UObject* Outer, int32 Width, int32 Height, EPixelFormat Format, TConstArrayView<TConstArrayView64<uint8>> Mips, bool bSrgb)
	{
		UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, Format, NAME_None, Mips[0]);
		if (!Texture)
		{
			return nullptr;
		}
		Texture->SRGB = bSrgb;
		Texture->AddressX = TA_Wrap;
		Texture->AddressY = TA_Wrap;
		FTexturePlatformData* PlatformData = Texture->GetPlatformData();
		for (int32 Level = 1; Level < Mips.Num(); ++Level)
		{
			FTexture2DMipMap* Mip = new FTexture2DMipMap(FMath::Max(1, Width >> Level), FMath::Max(1, Height >> Level), 1);
			PlatformData->Mips.Add(Mip);
			Mip->BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(Mip->BulkData.Realloc(Mips[Level].Num()), Mips[Level].GetData(), Mips[Level].Num());
			Mip->BulkData.Unlock();
		}
		Texture->UpdateResource();
		return Texture;
	}
}

// ---- textures -------------------------------------------------------------------------------------------------------------

UTexture2D* UKspPartAssets::GetTexture(const FString& Url, bool bAsNormalMap)
{
	if (Url.IsEmpty())
	{
		return nullptr;
	}
	const FString Key = Url + (bAsNormalMap ? TEXT("|normal") : TEXT("|colour"));
	if (UTexture2D** Found = TextureOfKey.Find(Key))
	{
		return *Found;
	}
	UTexture2D*& Slot = TextureOfKey.Add(Key, nullptr);
	const FString File = FKspModels::TextureFile(Url);
	TArray64<uint8> Bytes;
	if (File.IsEmpty() || !FFileHelper::LoadFileToArray(Bytes, *File))
	{
		++Stats.MissingTextures;
		UE_LOG(LogKspShell, Warning, TEXT("texture %s: no file in the game database"), *Url);
		return nullptr;
	}
	const FKspTextureHeader Header = FKspModels::DecodeTextureHeader(File, TConstArrayView<uint8>(Bytes.GetData(), int32(FMath::Min<int64>(Bytes.Num(), 4096))), Bytes.Num());
	UTexture2D* Texture = nullptr;
	// a normal map is data (x in alpha, y in green), a colour texture holds sRGB values
	const bool bSrgb = !bAsNormalMap;
	if (Header.Container == FKspTextureHeader::EContainer::Dds && Header.bLoadable && (Header.bDxt1 || Header.bDxt5))
	{
		// the DXT blocks as stored, every level of the file (KSP's files are in Unity's row order: FKspModelData's conventions)
		const int64 BlockBytes = Header.bDxt1 ? 8 : 16;
		TArray<TConstArrayView64<uint8>> Mips;
		int64 Offset = Header.DataOffset;
		for (int32 Level = 0; Level < Header.MipCount; ++Level)
		{
			const int64 W = FMath::Max(1, Header.Width >> Level), H = FMath::Max(1, Header.Height >> Level);
			const int64 Size = ((W + 3) / 4) * ((H + 3) / 4) * BlockBytes;
			if (Offset + Size > Bytes.Num())
			{
				break;
			}
			Mips.Emplace(Bytes.GetData() + Offset, Size);
			Offset += Size;
		}
		if (Mips.Num() > 0)
		{
			Texture = MakeTexture(this, Header.Width, Header.Height, Header.bDxt1 ? PF_DXT1 : PF_DXT5, Mips, bSrgb);
		}
	}
	else if (Header.bLoadable && Header.Width > 0 && Header.Height > 0)
	{
		TArray64<uint8> Bgra;
		if (Header.Container == FKspTextureHeader::EContainer::Mbm)
		{
			// raw RGB / RGBA rows in Unity's order (Texture2D.LoadRawTextureData)
			const int64 Step = Header.MbmBitsPerPixel == 32 ? 4 : 3;
			const int64 Pixels = int64(Header.Width) * Header.Height;
			if (Header.DataOffset + Pixels * Step <= Bytes.Num())
			{
				Bgra.SetNumUninitialized(Pixels * 4);
				const uint8* In = Bytes.GetData() + Header.DataOffset;
				for (int64 P = 0; P < Pixels; ++P, In += Step)
				{
					Bgra[P * 4] = In[2];
					Bgra[P * 4 + 1] = In[1];
					Bgra[P * 4 + 2] = In[0];
					Bgra[P * 4 + 3] = Step == 4 ? In[3] : 255;
				}
			}
		}
		else
		{
			// PNG, TGA, JPEG and TRUECOLOR decode with the top row first; Unity's row 0 is the bottom one
			FImage Image;
			IImageWrapperModule& ImageWrapper = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
			if (ImageWrapper.DecompressImage(Bytes.GetData(), Bytes.Num(), Image) && Image.SizeX == Header.Width && Image.SizeY == Header.Height)
			{
				Image.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
				Bgra = MoveTemp(Image.RawData);
				FlipRows(Bgra, Header.Width, Header.Height);
			}
		}
		if (Bgra.Num() == int64(Header.Width) * Header.Height * 4)
		{
			// GameDatabase.TextureInfo.normalMap (GameDatabase.cs:31): a readable texture that is not a normal map yet is converted on request
			if (bAsNormalMap && !Header.bNormalMap && Header.bReadable)
			{
				ToUnityNormalMap(Bgra);
			}
			const TConstArrayView64<uint8> Level0(Bgra.GetData(), Bgra.Num());
			Texture = MakeTexture(this, Header.Width, Header.Height, PF_B8G8R8A8, MakeArrayView(&Level0, 1), bSrgb);
		}
	}
	if (!Texture)
	{
		++Stats.UnusableTextures;
		UE_LOG(LogKspShell, Warning, TEXT("texture %s (%s): not usable: %s"), *Url, *File, Header.Error.IsEmpty() ? TEXT("format not handled") : *Header.Error);
		return nullptr;
	}
	++Stats.Textures;
	Keep.Add(Texture);
	Slot = Texture;
	return Texture;
}

// ---- meshes ---------------------------------------------------------------------------------------------------------------

UStaticMesh* UKspPartAssets::BuildStaticMesh(const FKspMeshData& In)
{
	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();
	TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> BinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector4f> Colors = Attributes.GetVertexInstanceColors();
	TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
	TPolygonGroupAttributesRef<FName> SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
	const bool bSecondUv = In.UV1.Num() == In.Positions.Num();
	UVs.SetNumChannels(bSecondUv ? 2 : 1);

	const int32 Count = In.Positions.Num();
	Description.ReserveNewVertices(Count);
	Description.ReserveNewVertexInstances(Count);
	TArray<FVertexInstanceID> Instances;
	Instances.Reserve(Count);
	for (int32 V = 0; V < Count; ++V)
	{
		const FVertexID Vertex = Description.CreateVertex();
		Positions[Vertex] = In.Positions[V];
		const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
		Instances.Add(Instance);
		// a mesh without normals (rare) is lit as if it faced up; one without tangents gets any tangent across its normal
		const FVector3f Normal = In.Normals.IsValidIndex(V) ? In.Normals[V].GetSafeNormal(UE_SMALL_NUMBER, FVector3f::UpVector) : FVector3f::UpVector;
		FVector3f Tangent;
		float Sign = 1.0f;
		if (In.Tangents.IsValidIndex(V))
		{
			Tangent = FVector3f(In.Tangents[V].X, In.Tangents[V].Y, In.Tangents[V].Z);
			Sign = In.Tangents[V].W < 0.0f ? -1.0f : 1.0f;
		}
		else
		{
			FVector3f Other;
			Normal.FindBestAxisVectors(Tangent, Other);
		}
		Normals[Instance] = Normal;
		Tangents[Instance] = Tangent;
		BinormalSigns[Instance] = Sign;
		Colors[Instance] = In.Colors.IsValidIndex(V) ? FVector4f(FLinearColor(In.Colors[V])) : FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
		UVs.Set(Instance, 0, In.UV0.IsValidIndex(V) ? In.UV0[V] : FVector2f::ZeroVector);
		if (bSecondUv)
		{
			UVs.Set(Instance, 1, In.UV1[V]);
		}
	}

	UStaticMesh* Mesh = NewObject<UStaticMesh>(this, NAME_None, RF_Transient);
	int64 Triangles = 0;
	for (int32 S = 0; S < In.Sections.Num(); ++S)
	{
		const FName SlotName(*FString::Printf(TEXT("Section%d"), S));
		const FPolygonGroupID Group = Description.CreatePolygonGroup();
		SlotNames[Group] = SlotName;
		Mesh->GetStaticMaterials().Add(FStaticMaterial(nullptr, SlotName, SlotName));
		const TArray<uint32>& Indices = In.Sections[S].Indices;
		for (int32 I = 0; I + 2 < Indices.Num(); I += 3)
		{
			const uint32 A = Indices[I], B = Indices[I + 1], C = Indices[I + 2];
			if (A == B || B == C || A == C)
			{
				continue;   // a triangle without area cannot be a polygon of the mesh description
			}
			Description.CreateTriangle(Group, { Instances[A], Instances[B], Instances[C] });
			++Triangles;
		}
	}
	if (Triangles == 0)
	{
		return nullptr;
	}
	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;            // the runtime path: attributes are used as given
	Params.bMarkPackageDirty = false;
	Params.bCommitMeshDescription = false;
	if (!Mesh->BuildFromMeshDescriptions({ &Description }, Params))
	{
		return nullptr;
	}
	++Stats.Meshes;
	Stats.Vertices += Count;
	Stats.Triangles += Triangles;
	Keep.Add(Mesh);
	return Mesh;
}

UKspPartAssets::FModelEntry& UKspPartAssets::GetModel(const FKspRigFile& File)
{
	if (TUniquePtr<FModelEntry>* Found = Models.Find(File.Path))
	{
		return **Found;
	}
	FModelEntry& Entry = *Models.Add(File.Path, MakeUnique<FModelEntry>());
	FString Error;
	if (!FKspModels::LoadModel(File.Path, File.ParentUrl, Entry.Data, Error))
	{
		++Stats.ModelFilesFailed;
		UE_LOG(LogKspShell, Warning, TEXT("model %s: %s"), *File.Path, *Error);
		return Entry;
	}
	Entry.bLoaded = true;
	++Stats.ModelFiles;
	Stats.SkinnedRenderers += Entry.Data.SkinnedRendererCount;
	Entry.Meshes.Reserve(Entry.Data.Meshes.Num());
	for (int32 M = 0; M < Entry.Data.Meshes.Num(); ++M)
	{
		FKspMeshData& Mesh = Entry.Data.Meshes[M];
		Entry.Meshes.Add(BuildStaticMesh(Mesh));
		// PartReader gives a node one MeshRenderer: the first one wins
		if (!Entry.MeshOfNode.Contains(Mesh.Node))
		{
			Entry.MeshOfNode.Add(Mesh.Node, M);
		}
		// the vertex data lives in the static mesh now; the sections' material ids stay
		Mesh.Positions.Empty();
		Mesh.Normals.Empty();
		Mesh.Tangents.Empty();
		Mesh.UV0.Empty();
		Mesh.UV1.Empty();
		Mesh.Colors.Empty();
		for (FKspMeshSection& Section : Mesh.Sections)
		{
			Section.Indices.Empty();
		}
	}
	return Entry;
}

// ---- materials ------------------------------------------------------------------------------------------------------------

UMaterialInterface* UKspPartAssets::GetMaster(const TCHAR* Name)
{
	const FName Key(Name);
	if (TObjectPtr<UMaterialInterface>* Found = Masters.Find(Key))
	{
		return *Found;
	}
	// generated by the KspCreateMaterials commandlet (Scripts\create_materials.ps1)
	const FString Path = FString::Printf(TEXT("/Game/Ksp/Materials/M_Ksp%s.M_Ksp%s"), Name, Name);
	UMaterialInterface* Master = LoadObject<UMaterialInterface>(nullptr, *Path);
	if (!Master)
	{
		UE_LOG(LogKspShell, Error, TEXT("master material %s is missing: run Scripts\\create_materials.ps1"), *Path);
	}
	Masters.Add(Key, Master);
	return Master;
}

UMaterialInterface* UKspPartAssets::GetMaterial(const FKspRigFile& File, const FModelEntry& Model, int32 MaterialId)
{
	FString Key = File.Path;
	for (const TPair<FString, FString>& Replacement : File.TextureReplacements)
	{
		Key += TEXT("|") + Replacement.Key + TEXT(">") + Replacement.Value;
	}
	Key += FString::Printf(TEXT("|%d"), MaterialId);
	if (UMaterialInterface** Found = MaterialOfKey.Find(Key))
	{
		return *Found;
	}
	UMaterialInterface*& Slot = MaterialOfKey.Add(Key, nullptr);
	static const FKspMaterialData NoMaterial;
	const FKspMaterialData& Data = Model.Data.Materials.IsValidIndex(MaterialId) ? Model.Data.Materials[MaterialId] : NoMaterial;
	const FKspShaderRule* Rule = FindShaderRule(Data.Shader);
	if (!Rule)
	{
		// counted for the slice criterion; drawn with the plain lit master so the part is not invisible
		++Stats.UnknownShaders.FindOrAdd(Data.Shader);
		static const FKspShaderRule Fallback = { TEXT(""), TEXT("Opaque"), false };
		Rule = &Fallback;
	}
	if (!Rule->Master)
	{
		return nullptr;
	}
	UMaterialInterface* Master = GetMaster(Rule->Master);
	if (!Master)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Master, this);
	++Stats.Materials;
	Keep.Add(Material);
	Slot = Material;

	// PartLoader.ReplaceTextures (PartLoader.cs:1932): a MODEL node's texture replacements apply to these four properties, by texture name
	auto Texture = [this, &Data, &File](const TCHAR* Property, bool bNormalMap) -> UTexture2D*
	{
		const FKspMaterialTexture* Found = Data.Textures.Find(FName(Property));
		if (!Found || Found->Url.IsEmpty())
		{
			return nullptr;
		}
		FString Url = Found->Url;
		for (const TPair<FString, FString>& Replacement : File.TextureReplacements)
		{
			if (Replacement.Key == Found->Name)
			{
				Url = Replacement.Value;
				break;
			}
		}
		return GetTexture(Url, bNormalMap || Found->bNormalMap);
	};
	auto Colour = [&Data](const TCHAR* Property, const FLinearColor& Default)
	{
		const FLinearColor* Found = Data.Colors.Find(FName(Property));
		return Found ? *Found : Default;
	};
	auto Scalar = [&Data](const TCHAR* Property, float Default)
	{
		const float* Found = Data.Scalars.Find(FName(Property));
		return Found ? *Found : Default;
	};

	if (UTexture2D* Main = Texture(TEXT("_MainTex"), false))
	{
		Material->SetTextureParameterValue(TEXT("MainTex"), Main);
	}
	if (const FKspMaterialTexture* Main = Data.Textures.Find(FName(TEXT("_MainTex"))))
	{
		Material->SetVectorParameterValue(TEXT("MainTexST"), FLinearColor(Main->Scale.X, Main->Scale.Y, Main->Offset.X, Main->Offset.Y));
	}
	if (UTexture2D* Bump = Texture(TEXT("_BumpMap"), true))
	{
		Material->SetTextureParameterValue(TEXT("BumpMap"), Bump);
	}
	if (UTexture2D* Emissive = Texture(TEXT("_Emissive"), false))
	{
		Material->SetTextureParameterValue(TEXT("Emissive"), Emissive);
	}
	Material->SetVectorParameterValue(TEXT("Color"), GammaToLinear(Colour(TEXT("_Color"), FLinearColor::White)));
	Material->SetVectorParameterValue(TEXT("EmissiveColor"), GammaToLinear(Colour(TEXT("_EmissiveColor"), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f))));
	Material->SetScalarParameterValue(TEXT("SpecLevel"), Rule->bSpecular ? Colour(TEXT("_SpecColor"), FLinearColor::Gray).GetLuminance() : 0.0f);
	Material->SetScalarParameterValue(TEXT("Shininess"), Scalar(TEXT("_Shininess"), 0.4f));
	Material->SetScalarParameterValue(TEXT("Cutoff"), Scalar(TEXT("_Cutoff"), 0.5f));
	Material->SetScalarParameterValue(TEXT("Opacity"), Scalar(TEXT("_Opacity"), 1.0f));
	return Material;
}

// ---- part templates -------------------------------------------------------------------------------------------------------

const FKspPartTemplate& UKspPartAssets::GetTemplate(uint32 ModelId)
{
	if (TUniquePtr<FKspPartTemplate>* Found = Templates.Find(ModelId))
	{
		return **Found;
	}
	const double T0 = FPlatformTime::Seconds();
	FKspPartTemplate& Template = *Templates.Add(ModelId, MakeUnique<FKspPartTemplate>());
	++Stats.Templates;
	FKspPartRig Rig;
	if (!FKspModels::GetPartRig(ModelId, Rig))
	{
		++Stats.PartsWithoutModel;
		UE_LOG(LogKspShell, Warning, TEXT("part %s has no readable model"), *FKspGame::ModelName(ModelId));
		return Template;
	}
	Template.Nodes.SetNum(Rig.Nodes.Num());
	for (int32 N = 0; N < Rig.Nodes.Num(); ++N)
	{
		const FKspRigNode& RigNode = Rig.Nodes[N];
		FKspPartNodeTemplate& Node = Template.Nodes[N];
		Node.Name = FName(*RigNode.Name);
		Node.Parent = RigNode.Parent;
		Node.Local = RigNode.Local;
		if (!Rig.Files.IsValidIndex(RigNode.File))
		{
			continue;
		}
		const FKspRigFile& File = Rig.Files[RigNode.File];
		FModelEntry& Model = GetModel(File);
		const int32* MeshIndex = Model.MeshOfNode.Find(RigNode.FileNode);
		if (!MeshIndex || !Model.Meshes[*MeshIndex])
		{
			continue;
		}
		const FKspMeshData& Mesh = Model.Data.Meshes[*MeshIndex];
		bool bDraws = false;
		Node.Materials.Reserve(Mesh.Sections.Num());
		for (const FKspMeshSection& Section : Mesh.Sections)
		{
			UMaterialInterface* Material = GetMaterial(File, Model, Section.Material);
			Node.Materials.Add(Material);
			bDraws |= Material != nullptr;
		}
		if (bDraws)
		{
			Node.Mesh = Model.Meshes[*MeshIndex];
			Node.bCastShadows = Mesh.bCastShadows;
		}
	}
	// keep a node when it or something below it draws (parents come before their children)
	for (int32 N = Template.Nodes.Num() - 1; N >= 0; --N)
	{
		FKspPartNodeTemplate& Node = Template.Nodes[N];
		Node.bKeep |= Node.Mesh != nullptr;
		if (Node.bKeep && Template.Nodes.IsValidIndex(Node.Parent))
		{
			Template.Nodes[Node.Parent].bKeep = true;
		}
	}
	Stats.BuildSeconds += FPlatformTime::Seconds() - T0;
	return Template;
}

FString UKspPartAssets::DescribeStats() const
{
	int32 Unknown = 0;
	FString Names;
	for (const TPair<FString, int32>& Shader : Stats.UnknownShaders)
	{
		Unknown += Shader.Value;
		Names += FString::Printf(TEXT(" '%s' x%d"), *Shader.Key, Shader.Value);
	}
	return FString::Printf(TEXT("%d part templates (%d without a model) from %d model files (%d failed): %d meshes, %lld vertices, %lld triangles, ")
		TEXT("%d skinned renderers not drawn, %d materials, %d textures, %d missing textures, %d unusable textures, %d materials with an unknown shader%s; built in %.2f s"),
		Stats.Templates, Stats.PartsWithoutModel, Stats.ModelFiles, Stats.ModelFilesFailed, Stats.Meshes, Stats.Vertices, Stats.Triangles, Stats.SkinnedRenderers,
		Stats.Materials, Stats.Textures, Stats.MissingTextures, Stats.UnusableTextures, Unknown, *Names, Stats.BuildSeconds);
}
