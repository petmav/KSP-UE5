using UnrealBuildTool;

public class KspUe5Editor : ModuleRules
{
	public KspUe5Editor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UnrealEd", "KspUe5" });
	}
}
