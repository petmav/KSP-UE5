using UnrealBuildTool;

public class KspUe5 : ModuleRules
{
	public KspUe5(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "ProceduralMeshComponent", "RenderCore", "RHI", "KspCoreBridge" });

		// Headers are included as "Spike/SpikeMath.h" etc., also from the KspUe5Editor module.
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
