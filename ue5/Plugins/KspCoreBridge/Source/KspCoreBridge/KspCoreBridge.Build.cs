using System;
using System.IO;
using UnrealBuildTool;

// Links kspcore.lib (engine-agnostic C++20 simulation core) into Unreal through a C ABI header.
//
// Requirements on the library so it links into UE 5.8 (see ue5/README.md "Linking the real core"):
//   MSVC 14.4x x64, /MD (release CRT DLL, never /MDd: UE uses the release CRT even in Debug editor builds),
//   _ITERATOR_DEBUG_LEVEL=0, C++20, no /GL, debug info embedded (/Z7) or the PDB next to the .lib.
//   Only C types cross the ABI: UE compiles with /GR- and _HAS_EXCEPTIONS=0.
public class KspCoreBridge : ModuleRules
{
	// ------------------------------------------------------------------------------------------------
	// ONE-LINE SWITCH. false = link the placeholder ue5/Stub/lib/kspcore.lib (one function).
	//                  true  = link the real core: <KSPCORE_ROOT or C:\Projects\KSP-UE5\core>\build\RelWithDebInfo\kspcore.lib
	//                          with headers from <root>\include. Override the lib path with env KSPCORE_LIB.
	private static readonly bool bUseRealCore = true;
	// ------------------------------------------------------------------------------------------------

	public KspCoreBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core" });

		string UeProjectRoot = Path.GetFullPath(Path.Combine(PluginDirectory, "..", ".."));   // ...\ue5
		string RepoRoot = Path.GetFullPath(Path.Combine(UeProjectRoot, ".."));                  // ...\KSP-UE5

		string IncludeDir;
		string LibFile;
		if (bUseRealCore)
		{
			string EnvRoot = Environment.GetEnvironmentVariable("KSPCORE_ROOT");
			string CoreRoot = string.IsNullOrEmpty(EnvRoot) ? Path.Combine(RepoRoot, "core") : EnvRoot;
			IncludeDir = Path.Combine(CoreRoot, "include");
			string EnvLib = Environment.GetEnvironmentVariable("KSPCORE_LIB");
			LibFile = string.IsNullOrEmpty(EnvLib) ? Path.Combine(CoreRoot, "build", "RelWithDebInfo", "kspcore.lib") : EnvLib;
			PublicDefinitions.Add("KSPCORE_STUB=0");
		}
		else
		{
			string StubRoot = Path.Combine(UeProjectRoot, "Stub");
			IncludeDir = Path.Combine(StubRoot, "include");
			LibFile = Path.Combine(StubRoot, "lib", "kspcore.lib");
			PublicDefinitions.Add("KSPCORE_STUB=1");
		}

		if (!File.Exists(LibFile))
		{
			throw new BuildException("KspCoreBridge: kspcore.lib not found at '" + LibFile + "'. " +
				(bUseRealCore ? "Build the core RelWithDebInfo first, or set KSPCORE_LIB." : "Run ue5/Scripts/build_stub.ps1."));
		}

		PublicSystemIncludePaths.Add(IncludeDir);
		PublicAdditionalLibraries.Add(LibFile);
	}
}
