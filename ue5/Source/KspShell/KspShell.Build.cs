using UnrealBuildTool;

// The shell: presents core's state (ue5/DESIGN.md 1.2). It talks to core only through the KspCoreBridge facade and does no ported maths.
public class KspShell : ModuleRules
{
	public KspShell(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "KspCoreBridge" });
		// part meshes and textures are built at run time from the game's own files (KspPartAssets)
		PrivateDependencyModuleNames.AddRange(new string[] { "MeshDescription", "StaticMeshDescription", "ImageCore", "ImageWrapper", "RenderCore", "RHI" });
		// headers sit next to their sources and are included by name, also from the KspUe5 tests
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
