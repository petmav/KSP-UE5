#include "KspCreateMaterialsCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSquareRoot.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogKspCreateMaterials, Log, All);

namespace
{
	const TCHAR* Folder = TEXT("/Game/Ksp/Materials/");

	// output pins of a texture sample and of a vector parameter
	constexpr int32 PinRgb = 0, PinR = 1, PinG = 2, PinB = 3, PinA = 4;

	template <class T>
	T* NewAsset(const TCHAR* Name)
	{
		UPackage* Package = CreatePackage(*(FString(Folder) + Name));
		Package->FullyLoad();
		return NewObject<T>(Package, FName(Name), RF_Public | RF_Standalone);
	}

	bool SaveAsset(UObject* Asset)
	{
		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Asset);
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		const bool bOk = UPackage::SavePackage(Package, Asset, *Filename, Args);
		UE_LOG(LogKspCreateMaterials, Display, TEXT("Saved %s -> %s : %s"), *Package->GetName(), *Filename, bOk ? TEXT("OK") : TEXT("FAILED"));
		return bOk;
	}

	/** A 4 x 4 texture of one value: the default of a texture parameter. */
	UTexture2D* MakeDefaultTexture(const TCHAR* Name, const FColor& Value, bool bSrgb)
	{
		UTexture2D* Texture = NewAsset<UTexture2D>(Name);
		TArray<FColor> Pixels;
		Pixels.Init(Value, 16);
		Texture->Source.Init(4, 4, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
		Texture->SRGB = bSrgb;
		// uncompressed: a data texture must come back exactly
		Texture->CompressionSettings = bSrgb ? TC_EditorIcon : TC_VectorDisplacementmap;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->PostEditChange();
		return Texture;
	}

	struct FGraph
	{
		UMaterial* Material;
		int32 Row = 0;

		template <class T>
		T* Add()
		{
			T* Expression = NewObject<T>(Material);
			Expression->MaterialExpressionEditorX = -400 - 220 * (Row % 6);
			Expression->MaterialExpressionEditorY = 120 * (Row / 6);
			++Row;
			Material->GetExpressionCollection().AddExpression(Expression);
			return Expression;
		}

		UMaterialExpression* Scalar(const TCHAR* Name, float Default)
		{
			UMaterialExpressionScalarParameter* P = Add<UMaterialExpressionScalarParameter>();
			P->ParameterName = Name;
			P->DefaultValue = Default;
			return P;
		}

		UMaterialExpressionVectorParameter* Vector(const TCHAR* Name, const FLinearColor& Default)
		{
			UMaterialExpressionVectorParameter* P = Add<UMaterialExpressionVectorParameter>();
			P->ParameterName = Name;
			P->DefaultValue = Default;
			return P;
		}

		UMaterialExpression* Constant(float Value)
		{
			UMaterialExpressionConstant* C = Add<UMaterialExpressionConstant>();
			C->R = Value;
			return C;
		}

		UMaterialExpression* Texture(const TCHAR* Name, UTexture* Default, EMaterialSamplerType Sampler, UMaterialExpression* Uv)
		{
			UMaterialExpressionTextureSampleParameter2D* T = Add<UMaterialExpressionTextureSampleParameter2D>();
			T->ParameterName = Name;
			T->Texture = Default;
			T->SamplerType = Sampler;
			T->Coordinates.Connect(0, Uv);
			return T;
		}

		UMaterialExpression* Mul(UMaterialExpression* A, int32 PinOfA, UMaterialExpression* B, int32 PinOfB)
		{
			UMaterialExpressionMultiply* M = Add<UMaterialExpressionMultiply>();
			M->A.Connect(PinOfA, A);
			M->B.Connect(PinOfB, B);
			return M;
		}

		UMaterialExpression* Sum(UMaterialExpression* A, int32 PinOfA, UMaterialExpression* B, int32 PinOfB)
		{
			UMaterialExpressionAdd* S = Add<UMaterialExpressionAdd>();
			S->A.Connect(PinOfA, A);
			S->B.Connect(PinOfB, B);
			return S;
		}

		UMaterialExpression* Sub(UMaterialExpression* A, int32 PinOfA, UMaterialExpression* B, int32 PinOfB)
		{
			UMaterialExpressionSubtract* S = Add<UMaterialExpressionSubtract>();
			S->A.Connect(PinOfA, A);
			S->B.Connect(PinOfB, B);
			return S;
		}

		UMaterialExpression* Append(UMaterialExpression* A, int32 PinOfA, UMaterialExpression* B, int32 PinOfB)
		{
			UMaterialExpressionAppendVector* V = Add<UMaterialExpressionAppendVector>();
			V->A.Connect(PinOfA, A);
			V->B.Connect(PinOfB, B);
			return V;
		}
	};

	enum class EKind { Opaque, Masked, Translucent, Additive, Unlit, UnlitTranslucent };

	/**
	 * One master material. Parameters (set per part material by UKspPartAssets from the .mu material chunk):
	 *   MainTex, MainTexST (scale xy, offset zw), Color           _MainTex with its tiling, _Color
	 *   BumpMap                                                  _BumpMap in Unity's normal-map layout: x in alpha, y in green
	 *   Emissive, EmissiveColor                                  _Emissive, _EmissiveColor (rgb scaled by its alpha)
	 *   SpecLevel, Shininess                                     luminance of _SpecColor (0 for shaders without specular), _Shininess
	 *   Cutoff, Opacity                                          _Cutoff (alpha test), _Opacity
	 * KSP's shaders are Blinn-Phong in gamma space; this is a physically based stand-in with the same inputs (DESIGN open decision O7):
	 * the gloss map is the alpha of _MainTex, as in KSP/Specular.
	 */
	UMaterial* MakeMaster(const TCHAR* Name, EKind Kind, UTexture2D* White, UTexture2D* Black, UTexture2D* FlatNormal, UMaterialParameterCollection* Collection)
	{
		UMaterial* Material = NewAsset<UMaterial>(Name);
		FGraph G{ Material };
		const bool bUnlit = Kind == EKind::Additive || Kind == EKind::Unlit || Kind == EKind::UnlitTranslucent;
		UMaterialEditorOnlyData* Out = Material->GetEditorOnlyData();

		// uv = uv0 * scale + offset
		UMaterialExpressionVectorParameter* St = G.Vector(TEXT("MainTexST"), FLinearColor(1.0f, 1.0f, 0.0f, 0.0f));
		UMaterialExpression* Uv = G.Sum(G.Mul(G.Add<UMaterialExpressionTextureCoordinate>(), 0, G.Append(St, PinR, St, PinG), 0), 0, G.Append(St, PinB, St, PinA), 0);

		UMaterialExpression* Main = G.Texture(TEXT("MainTex"), White, SAMPLERTYPE_Color, Uv);
		UMaterialExpressionVectorParameter* Color = G.Vector(TEXT("Color"), FLinearColor::White);
		UMaterialExpression* BaseColor = G.Mul(Main, PinRgb, Color, PinRgb);
		UMaterialExpression* Alpha = G.Mul(G.Mul(Main, PinA, Color, PinA), 0, G.Scalar(TEXT("Opacity"), 1.0f), 0);

		UMaterialExpression* EmissiveTexture = G.Texture(TEXT("Emissive"), Black, SAMPLERTYPE_Color, Uv);
		UMaterialExpressionVectorParameter* EmissiveColor = G.Vector(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		UMaterialExpression* Emissive = G.Mul(G.Mul(EmissiveTexture, PinRgb, EmissiveColor, PinRgb), 0, EmissiveColor, PinA);

		if (bUnlit)
		{
			Material->SetShadingModel(MSM_Unlit);
			// an additive surface adds its colour scaled by its alpha
			UMaterialExpression* Lit = Kind == EKind::Additive ? G.Mul(BaseColor, 0, Alpha, 0) : BaseColor;
			Out->EmissiveColor.Connect(0, G.Sum(Lit, 0, Emissive, 0));
		}
		else
		{
			Out->BaseColor.Connect(0, BaseColor);
			// the flat ambient light of the scene (KSP: RenderSettings.ambientLight), a scene-wide value
			UMaterialExpressionCollectionParameter* Ambient = G.Add<UMaterialExpressionCollectionParameter>();
			Ambient->Collection = Collection;
			Ambient->ParameterName = TEXT("Ambient");
			Ambient->ParameterId = Collection->GetParameterId(TEXT("Ambient"));
			UMaterialExpressionComponentMask* AmbientRgb = G.Add<UMaterialExpressionComponentMask>();   // the parameter is a float4
			AmbientRgb->Input.Connect(0, Ambient);
			AmbientRgb->R = AmbientRgb->G = AmbientRgb->B = true;
			AmbientRgb->A = false;
			Out->EmissiveColor.Connect(0, G.Sum(Emissive, 0, G.Mul(BaseColor, 0, AmbientRgb, 0), 0));

			// normal = (a, g) * 2 - 1, z rebuilt
			UMaterialExpression* Bump = G.Texture(TEXT("BumpMap"), FlatNormal, SAMPLERTYPE_LinearColor, Uv);
			UMaterialExpression* Xy = G.Sub(G.Mul(G.Append(Bump, PinA, Bump, PinG), 0, G.Constant(2.0f), 0), 0, G.Constant(1.0f), 0);
			UMaterialExpressionDotProduct* Dot = G.Add<UMaterialExpressionDotProduct>();
			Dot->A.Connect(0, Xy);
			Dot->B.Connect(0, Xy);
			UMaterialExpressionOneMinus* OneMinus = G.Add<UMaterialExpressionOneMinus>();
			OneMinus->Input.Connect(0, Dot);
			UMaterialExpressionSaturate* Saturate = G.Add<UMaterialExpressionSaturate>();
			Saturate->Input.Connect(0, OneMinus);
			UMaterialExpressionSquareRoot* Z = G.Add<UMaterialExpressionSquareRoot>();
			Z->Input.Connect(0, Saturate);
			Out->Normal.Connect(0, G.Append(Xy, 0, Z, 0));

			Out->Specular.Connect(0, G.Mul(G.Mul(G.Scalar(TEXT("SpecLevel"), 0.0f), 0, Main, PinA), 0, G.Constant(0.5f), 0));
			UMaterialExpressionLinearInterpolate* Roughness = G.Add<UMaterialExpressionLinearInterpolate>();
			Roughness->A.Connect(0, G.Constant(0.9f));
			Roughness->B.Connect(0, G.Constant(0.3f));
			Roughness->Alpha.Connect(0, G.Scalar(TEXT("Shininess"), 0.4f));
			Out->Roughness.Connect(0, Roughness);
		}

		switch (Kind)
		{
		case EKind::Masked:
			Material->BlendMode = BLEND_Masked;
			Material->OpacityMaskClipValue = 0.5f;
			// drawn where alpha > Cutoff
			Out->OpacityMask.Connect(0, G.Sum(G.Sub(Main, PinA, G.Scalar(TEXT("Cutoff"), 0.5f), 0), 0, G.Constant(0.5f), 0));
			break;
		case EKind::Translucent:
		case EKind::UnlitTranslucent:
			Material->BlendMode = BLEND_Translucent;
			Out->Opacity.Connect(0, Alpha);
			break;
		case EKind::Additive:
			Material->BlendMode = BLEND_Additive;
			break;
		default:
			break;
		}
		Material->PostEditChange();
		return Material;
	}
}

UKspCreateMaterialsCommandlet::UKspCreateMaterialsCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UKspCreateMaterialsCommandlet::Main(const FString& Params)
{
	bool bOk = true;
	UTexture2D* White = MakeDefaultTexture(TEXT("T_KspWhite"), FColor(255, 255, 255, 255), true);
	UTexture2D* Black = MakeDefaultTexture(TEXT("T_KspBlack"), FColor(0, 0, 0, 255), true);
	// Unity's normal-map layout of the flat normal: x (alpha) = y (green) = 0.5
	UTexture2D* FlatNormal = MakeDefaultTexture(TEXT("T_KspFlatNormal"), FColor(128, 128, 128, 128), false);
	bOk &= SaveAsset(White);
	bOk &= SaveAsset(Black);
	bOk &= SaveAsset(FlatNormal);

	UMaterialParameterCollection* Collection = NewAsset<UMaterialParameterCollection>(TEXT("MPC_Ksp"));
	FCollectionVectorParameter Ambient;
	Ambient.ParameterName = TEXT("Ambient");
	Ambient.Id = FGuid(0x4B535031, 0x416D6269, 0x656E7400, 0x00000001);   // fixed: regenerating must not orphan the materials' references
	Ambient.DefaultValue = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
	Collection->VectorParameters.Add(Ambient);
	Collection->PostEditChange();
	bOk &= SaveAsset(Collection);

	const TPair<const TCHAR*, EKind> Masters[] = {
		{ TEXT("M_KspOpaque"), EKind::Opaque },
		{ TEXT("M_KspMasked"), EKind::Masked },
		{ TEXT("M_KspTranslucent"), EKind::Translucent },
		{ TEXT("M_KspAdditive"), EKind::Additive },
		{ TEXT("M_KspUnlit"), EKind::Unlit },
		{ TEXT("M_KspUnlitTranslucent"), EKind::UnlitTranslucent },
	};
	for (const TPair<const TCHAR*, EKind>& Master : Masters)
	{
		bOk &= SaveAsset(MakeMaster(Master.Key, Master.Value, White, Black, FlatNormal, Collection));
	}
	return bOk ? 0 : 1;
}
