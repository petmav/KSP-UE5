#pragma once

#include "CoreMinimal.h"

/**
 * Part models through the facade (WP-UE-10): the model hierarchy of a part (kspcore_game.h "part models"), the render side of a
 * `.mu` file (kspcore_model.h) and texture headers. Everything is converted here, once, to Unreal's axes and centimetres (FKspAxis);
 * the shell never sees Unity values.
 *
 * Conventions of the converted data:
 *  - positions are centimetres in the frame of the node that owns the mesh; normals and tangents are permuted like directions;
 *  - triangle order is flipped (Unity front faces are clockwise, Unreal's the other way; the axis map itself keeps handedness);
 *  - texture coordinates are Unity's, unchanged. A texture must therefore be uploaded with Unity's row order (row 0 = v 0): KSP's DDS
 *    and MBM files already are, decoded PNG / TGA images are flipped by the loader (KspShell).
 */

/** One node of a part's model hierarchy (PartLoader.CompileModel): node 0 is "model", the child of the part transform. */
struct FKspRigNode
{
	FString Name;
	int32 Parent = INDEX_NONE;
	FTransform Local = FTransform::Identity;
	/** Index into FKspPartRig::Files, INDEX_NONE for the synthetic "model" object. */
	int32 File = INDEX_NONE;
	/** The node's id inside that .mu (FKspMeshData::Node of the renderers it draws). */
	int32 FileNode = INDEX_NONE;
};

struct FKspRigFile
{
	/** The .mu file. */
	FString Path;
	/** GameDatabase url of its directory. */
	FString ParentUrl;
	/** MODEL texture = <original name> , <new url> entries (PartLoader.cs:1932 ReplaceTextures). */
	TArray<TPair<FString, FString>> TextureReplacements;
};

struct FKspPartRig
{
	TArray<FKspRigFile> Files;
	TArray<FKspRigNode> Nodes;
};

/** One triangle list of a mesh and the model material that draws it (INDEX_NONE: the renderer lists none for it). */
struct FKspMeshSection
{
	TArray<uint32> Indices;
	int32 Material = INDEX_NONE;
};

/** A MeshRenderer of a .mu node with the mesh of that node's MeshFilter. */
struct FKspMeshData
{
	/** The .mu node that owns the renderer. */
	int32 Node = INDEX_NONE;
	TArray<FVector3f> Positions;
	/** Empty when the file has none. */
	TArray<FVector3f> Normals;
	/** xyz and the bitangent sign w, Unity's convention: bitangent = cross(normal, tangent) * w. Empty when the file has none. */
	TArray<FVector4f> Tangents;
	TArray<FVector2f> UV0;
	TArray<FVector2f> UV1;
	TArray<FColor> Colors;
	TArray<FKspMeshSection> Sections;
	bool bCastShadows = true;
	bool bReceiveShadows = true;
};

struct FKspMaterialTexture
{
	/** GameDatabase texture url (no extension); empty: the property has no texture. */
	FString Url;
	/** The texture's file name without extension, as the model stores it (what a MODEL texture replacement matches). */
	FString Name;
	/** The model asks for it as a normal map (GameDatabase.GetTexture(url, asNormalMap)). */
	bool bNormalMap = false;
	FVector2f Scale = FVector2f(1.0f, 1.0f);
	FVector2f Offset = FVector2f::ZeroVector;
};

/** A material chunk: the Unity shader name and its properties by their Unity names ("_MainTex", "_Color", "_Shininess" ...). */
struct FKspMaterialData
{
	FString Name;
	FString Shader;
	TMap<FName, FLinearColor> Colors;
	TMap<FName, float> Scalars;
	TMap<FName, FKspMaterialTexture> Textures;
};

struct FKspModelData
{
	int32 NodeCount = 0;
	TArray<FKspMeshData> Meshes;
	TArray<FKspMaterialData> Materials;
	/** Skinned renderers are not converted yet (few stock parts; not in the slice). */
	int32 SkinnedRendererCount = 0;
};

/** kspcore_texture_header: what the game's loader makes of a texture file. */
struct FKspTextureHeader
{
	enum class EContainer : uint8 { Unknown, Dds, Mbm, Png, Jpeg, Tga, TrueColor };
	EContainer Container = EContainer::Unknown;
	bool bValid = false;
	/** The game's loader returns a texture for this file. */
	bool bLoadable = false;
	int32 Width = 0;
	int32 Height = 0;
	/** DDS: the block format stored in the file. */
	bool bDxt1 = false;
	bool bDxt5 = false;
	/** Levels stored in the file (DDS), 1 otherwise. */
	int32 MipCount = 1;
	/** GameDatabase.TextureInfo.isNormalMap: the pixels already have Unity's normal-map layout (x in alpha, y in green). */
	bool bNormalMap = false;
	/** TextureInfo.isReadable: GameDatabase converts it on a normal-map request (BitmapToUnityNormalMap). */
	bool bReadable = false;
	int32 MbmBitsPerPixel = 0;
	/** First byte of the pixel data. */
	int64 DataOffset = 0;
	FString Error;
};

class KSPCOREBRIDGE_API FKspModels
{
public:
	/** The rig of the part with that snapshot model id; false when the part has no readable model. */
	static bool GetPartRig(uint32 ModelId, FKspPartRig& Out);
	/** The render side of a .mu file, converted. False with a message when the file cannot be read. */
	static bool LoadModel(const FString& Path, const FString& ParentUrl, FKspModelData& Out, FString& OutError);
	/** The file GameDatabase.GetTexture(url) reads; empty when the database has no such texture. */
	static FString TextureFile(const FString& Url);
	/** Decodes the header of a texture file's bytes (the whole file or its start). */
	static FKspTextureHeader DecodeTextureHeader(const FString& FileName, TConstArrayView<uint8> Data, int64 FileSize);
};
