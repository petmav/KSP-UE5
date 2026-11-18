#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "KspCreateMaterialsCommandlet.generated.h"

/**
 * Generates the shell's master materials from code (ue5/DESIGN.md 1.2, 3.4), so they stay reproducible and agent-legible:
 *   /Game/Ksp/Materials/M_KspOpaque, M_KspMasked, M_KspTranslucent, M_KspAdditive, M_KspUnlit, M_KspUnlitTranslucent (parts)
 *   /Game/Ksp/Materials/M_KspTerrain (terrain quads: the PQS vertex colour)
 *   /Game/Ksp/Materials/MPC_Ksp (scene-wide values: Ambient), T_KspWhite, T_KspBlack, T_KspFlatNormal (parameter defaults)
 * They hold no game data: every texture and value is set per part material at run time (UKspPartAssets) from the game's files.
 * Run: UnrealEditor-Cmd.exe <uproject> -run=KspCreateMaterials -unattended -nopause   (Scripts\create_materials.ps1)
 */
UCLASS()
class UKspCreateMaterialsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UKspCreateMaterialsCommandlet();
	virtual int32 Main(const FString& Params) override;
};
