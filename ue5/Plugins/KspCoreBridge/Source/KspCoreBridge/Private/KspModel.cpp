#include "KspModel.h"

#include "KspCoreBridge.h"
#include "KspSim.h"

#include "kspcore/kspcore_c.h"
#include "kspcore/kspcore_model.h"

static_assert(int32(FKspTextureHeader::EContainer::Dds) == KSPCORE_TEX_DDS && int32(FKspTextureHeader::EContainer::Mbm) == KSPCORE_TEX_MBM &&
	int32(FKspTextureHeader::EContainer::Png) == KSPCORE_TEX_PNG && int32(FKspTextureHeader::EContainer::Jpeg) == KSPCORE_TEX_JPEG &&
	int32(FKspTextureHeader::EContainer::Tga) == KSPCORE_TEX_TGA && int32(FKspTextureHeader::EContainer::TrueColor) == KSPCORE_TEX_TRUECOLOR,
	"FKspTextureHeader::EContainer mirrors KSPCORE_TEX_*");

namespace
{
	FVector3f DirectionToUe(const float* V) { return FVector3f(V[2], V[0], V[1]); }
	FVector3f PositionToUe(const float* V) { return DirectionToUe(V) * float(FKspAxis::CmPerMetre); }

	/** Sets struct_size and zeroes the rest: the protocol of every kspcore info struct. */
	template <class T>
	T Sized()
	{
		T Value;
		FMemory::Memzero(Value);
		Value.struct_size = sizeof(T);
		return Value;
	}

	struct FModelHandle
	{
		kspcore_model* Model = nullptr;
		~FModelHandle() { kspcore_model_release(Model); }
	};
}

bool FKspModels::GetPartRig(uint32 ModelId, FKspPartRig& Out)
{
	Out = FKspPartRig();
	FKspFpGuard Guard;
	const int32 NodeCount = kspcore_game_part_rig_node_count(ModelId);
	if (NodeCount <= 0)
	{
		return false;
	}
	const int32 FileCount = kspcore_game_part_model_file_count(ModelId);
	Out.Files.Reserve(FileCount);
	for (int32 F = 0; F < FileCount; ++F)
	{
		kspcore_part_model_file File = Sized<kspcore_part_model_file>();
		if (kspcore_game_part_model_file(ModelId, F, &File) != 1)
		{
			return false;
		}
		FKspRigFile& OutFile = Out.Files.AddDefaulted_GetRef();
		OutFile.Path = UTF8_TO_TCHAR(File.path);
		OutFile.ParentUrl = UTF8_TO_TCHAR(File.parent_url);
		for (int32 R = 0; R < File.texture_replacement_count; ++R)
		{
			const char* Original = nullptr;
			const char* Url = nullptr;
			if (kspcore_game_part_texture_replacement(ModelId, F, R, &Original, &Url) == 1)
			{
				OutFile.TextureReplacements.Emplace(FString(UTF8_TO_TCHAR(Original)), FString(UTF8_TO_TCHAR(Url)));
			}
		}
	}
	Out.Nodes.Reserve(NodeCount);
	for (int32 N = 0; N < NodeCount; ++N)
	{
		kspcore_part_rig_node Node = Sized<kspcore_part_rig_node>();
		if (kspcore_game_part_rig_node(ModelId, N, &Node) != 1)
		{
			return false;
		}
		FKspRigNode& OutNode = Out.Nodes.AddDefaulted_GetRef();
		OutNode.Name = UTF8_TO_TCHAR(Node.name);
		OutNode.Parent = Node.parent;
		// a scale is a size along the node's own axes: axis map, no unit factor
		OutNode.Local = FTransform(
			FKspAxis::RotationToUe(FQuat(Node.rotation[0], Node.rotation[1], Node.rotation[2], Node.rotation[3])).GetNormalized(),
			FKspAxis::PositionToUe(FVector(Node.position[0], Node.position[1], Node.position[2])),
			FKspAxis::DirectionToUe(FVector(Node.scale[0], Node.scale[1], Node.scale[2])));
		OutNode.File = Node.file;
		OutNode.FileNode = Node.file_node;
	}
	return true;
}

bool FKspModels::LoadModel(const FString& Path, const FString& ParentUrl, FKspModelData& Out, FString& OutError)
{
	Out = FKspModelData();
	FKspFpGuard Guard;
	FModelHandle Handle;
	Handle.Model = kspcore_model_load_file(TCHAR_TO_UTF8(*Path), TCHAR_TO_UTF8(*ParentUrl));
	if (!Handle.Model)
	{
		OutError = UTF8_TO_TCHAR(kspcore_model_last_error());
		return false;
	}
	const kspcore_model* Model = Handle.Model;
	kspcore_model_info Info = Sized<kspcore_model_info>();
	kspcore_model_get_info(Model, &Info);
	Out.NodeCount = Info.node_count;

	// textures of the model: url, name, kind
	TArray<kspcore_model_texture> Textures;
	Textures.Reserve(Info.texture_count);
	for (int32 T = 0; T < Info.texture_count; ++T)
	{
		kspcore_model_texture& Texture = Textures.Add_GetRef(Sized<kspcore_model_texture>());
		kspcore_model_get_texture(Model, T, &Texture);
	}

	Out.Materials.Reserve(Info.material_count);
	for (int32 M = 0; M < Info.material_count; ++M)
	{
		kspcore_model_material Material = Sized<kspcore_model_material>();
		kspcore_model_get_material(Model, M, &Material);
		FKspMaterialData& OutMaterial = Out.Materials.AddDefaulted_GetRef();
		OutMaterial.Name = UTF8_TO_TCHAR(Material.name);
		OutMaterial.Shader = UTF8_TO_TCHAR(Material.shader_name);
		for (int32 P = 0; P < Material.property_count; ++P)
		{
			kspcore_model_property Property = Sized<kspcore_model_property>();
			if (kspcore_model_get_property(Model, M, P, &Property) != 1)
			{
				continue;
			}
			const FName Name(UTF8_TO_TCHAR(Property.name));
			switch (Property.kind)
			{
			case KSPCORE_PROP_COLOR:
			case KSPCORE_PROP_VECTOR:
				OutMaterial.Colors.Add(Name, FLinearColor(Property.value[0], Property.value[1], Property.value[2], Property.value[3]));
				break;
			case KSPCORE_PROP_FLOAT:
			case KSPCORE_PROP_RANGE:
				OutMaterial.Scalars.Add(Name, Property.value[0]);
				break;
			case KSPCORE_PROP_TEXTURE:
			{
				FKspMaterialTexture& OutTexture = OutMaterial.Textures.Add(Name);
				if (Textures.IsValidIndex(Property.texture_id))
				{
					const kspcore_model_texture& Texture = Textures[Property.texture_id];
					OutTexture.Url = UTF8_TO_TCHAR(Texture.url);
					OutTexture.Name = UTF8_TO_TCHAR(Texture.name);
					OutTexture.bNormalMap = Texture.type == 1;
				}
				OutTexture.Scale = FVector2f(Property.tex_scale[0], Property.tex_scale[1]);
				OutTexture.Offset = FVector2f(Property.tex_offset[0], Property.tex_offset[1]);
				break;
			}
			default:
				break;
			}
		}
	}

	for (int32 R = 0; R < Info.renderer_count; ++R)
	{
		kspcore_model_renderer Renderer = Sized<kspcore_model_renderer>();
		kspcore_model_get_renderer(Model, R, &Renderer);
		if (Renderer.kind != KSPCORE_RENDERER_MESH)
		{
			++Out.SkinnedRendererCount;
			continue;
		}
		kspcore_model_mesh Mesh = Sized<kspcore_model_mesh>();
		if (Renderer.mesh < 0 || kspcore_model_get_mesh(Model, Renderer.mesh, &Mesh) != 1 || Mesh.vertex_count <= 0 || !Mesh.vertices)
		{
			continue;   // a MeshRenderer without a MeshFilter mesh draws nothing
		}
		FKspMeshData& OutMesh = Out.Meshes.AddDefaulted_GetRef();
		OutMesh.Node = Renderer.node;
		OutMesh.bCastShadows = Renderer.cast_shadows != 0;
		OutMesh.bReceiveShadows = Renderer.receive_shadows != 0;
		const int32 Count = Mesh.vertex_count;
		OutMesh.Positions.SetNumUninitialized(Count);
		for (int32 V = 0; V < Count; ++V)
		{
			OutMesh.Positions[V] = PositionToUe(Mesh.vertices + 3 * V);
		}
		if (Mesh.normals)
		{
			OutMesh.Normals.SetNumUninitialized(Count);
			for (int32 V = 0; V < Count; ++V)
			{
				OutMesh.Normals[V] = DirectionToUe(Mesh.normals + 3 * V);
			}
		}
		if (Mesh.tangents)
		{
			OutMesh.Tangents.SetNumUninitialized(Count);
			for (int32 V = 0; V < Count; ++V)
			{
				const float* T = Mesh.tangents + 4 * V;
				OutMesh.Tangents[V] = FVector4f(DirectionToUe(T), T[3]);
			}
		}
		if (Mesh.uv)
		{
			OutMesh.UV0.SetNumUninitialized(Count);
			FMemory::Memcpy(OutMesh.UV0.GetData(), Mesh.uv, sizeof(FVector2f) * Count);
		}
		if (Mesh.uv2)
		{
			OutMesh.UV1.SetNumUninitialized(Count);
			FMemory::Memcpy(OutMesh.UV1.GetData(), Mesh.uv2, sizeof(FVector2f) * Count);
		}
		if (Mesh.colors32)
		{
			OutMesh.Colors.SetNumUninitialized(Count);
			for (int32 V = 0; V < Count; ++V)
			{
				const uint8* C = Mesh.colors32 + 4 * V;
				OutMesh.Colors[V] = FColor(C[0], C[1], C[2], C[3]);
			}
		}
		for (int32 S = 0; S < Mesh.submesh_count; ++S)
		{
			int32 IndexCount = 0;
			const int32_t* Indices = kspcore_model_mesh_submesh(Model, Renderer.mesh, S, &IndexCount);
			if (!Indices || IndexCount < 3)
			{
				continue;
			}
			FKspMeshSection& Section = OutMesh.Sections.AddDefaulted_GetRef();
			Section.Material = S < Renderer.material_slot_count ? Renderer.material_slots[S] : INDEX_NONE;
			Section.Indices.Reserve(IndexCount);
			for (int32 I = 0; I + 2 < IndexCount; I += 3)
			{
				// skip triangles that point outside the vertex array (a broken file must not crash the mesh build)
				if (uint32(Indices[I]) >= uint32(Count) || uint32(Indices[I + 1]) >= uint32(Count) || uint32(Indices[I + 2]) >= uint32(Count))
				{
					continue;
				}
				Section.Indices.Add(Indices[I]);
				Section.Indices.Add(Indices[I + 2]);
				Section.Indices.Add(Indices[I + 1]);
			}
		}
	}
	return true;
}

FString FKspModels::TextureFile(const FString& Url)
{
	FKspFpGuard Guard;
	const char* File = kspcore_game_texture_file(TCHAR_TO_UTF8(*Url));
	return File ? FString(UTF8_TO_TCHAR(File)) : FString();
}

FKspTextureHeader FKspModels::DecodeTextureHeader(const FString& FileName, TConstArrayView<uint8> Data, int64 FileSize)
{
	FKspFpGuard Guard;
	kspcore_texture_header Header = Sized<kspcore_texture_header>();
	kspcore_texture_decode_header(TCHAR_TO_UTF8(*FileName), Data.GetData(), Data.Num(), uint64(FileSize), &Header);
	FKspTextureHeader Out;
	Out.Container = FKspTextureHeader::EContainer(Header.container);
	Out.bValid = Header.valid != 0;
	Out.bLoadable = Header.loadable != 0;
	Out.Width = Header.width;
	Out.Height = Header.height;
	Out.bDxt1 = Header.source_format == KSPCORE_TEXFMT_DXT1;
	Out.bDxt5 = Header.source_format == KSPCORE_TEXFMT_DXT5;
	Out.MipCount = Header.container == KSPCORE_TEX_DDS ? FMath::Max<int32>(1, int32(Header.dds_mip_map_count)) : 1;
	Out.bNormalMap = Header.is_normal_map != 0;
	Out.bReadable = Header.is_readable != 0;
	Out.MbmBitsPerPixel = FMath::Max(0, Header.mbm_bits_per_pixel);
	Out.DataOffset = int64(Header.data_offset);
	Out.Error = UTF8_TO_TCHAR(Header.error);
	return Out;
}
