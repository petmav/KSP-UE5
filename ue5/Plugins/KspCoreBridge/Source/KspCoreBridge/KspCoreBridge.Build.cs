using System;
using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

// Links kspcore.lib (engine-agnostic C++20 simulation core) and Jolt.lib (the physics library kspcore.lib depends on) into
// Unreal through a C ABI header.
//
// Real-core mode reads the LINK MANIFEST that the core build writes next to kspcore.lib
// (core/src/CMakeLists.txt, <core>/build/RelWithDebInfo/kspcore_link.txt): the libraries in link order, the include directory and
// the preprocessor definitions that must match (Jolt's JPH_* configuration). A new core dependency therefore never needs an edit
// here. Format: "key=value" lines (manifest_version, config, include, lib, define), '#' comments.
//
// Requirements on the libraries so they link into UE 5.8 (see ue5/README.md "Linking the real core"):
//   MSVC 14.4x x64, /MD (release CRT DLL, never /MDd: UE uses the release CRT even in Debug editor builds),
//   _ITERATOR_DEBUG_LEVEL=0, C++20, no /GL, debug info embedded (/Z7) or the PDB next to the .lib.
//   Only C types cross the ABI: UE compiles with /GR- and _HAS_EXCEPTIONS=0.
// Jolt is built with AVX2 (core/cmake/ksp_jolt.cmake): the machine running the game needs an AVX2 CPU. Jolt's own
// objects keep their flags; nothing from Jolt's headers is compiled by UE.
public class KspCoreBridge : ModuleRules
{
	// ------------------------------------------------------------------------------------------------
	// ONE-LINE SWITCH. false = link the placeholder ue5/Stub/lib/kspcore.lib (one function).
	//                  true  = link the real core through its link manifest:
	//                          <KSPCORE_ROOT or C:\Projects\KSP-UE5\core>\build\RelWithDebInfo\kspcore_link.txt
	//                          Override the manifest with env KSPCORE_LINK_MANIFEST (a full file path).
	private static readonly bool bUseRealCore = true;
	// ------------------------------------------------------------------------------------------------

	private const int SupportedManifestVersion = 1;

	public KspCoreBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// The facade converts and guards the FP environment; it must behave the same in the Editor (/fp:precise) and the
		// Game target (UBT default /fp:fast, BuildSettingsVersion V7). ue5/DESIGN.md 1.5 point 3.
		FPSemantics = FPSemanticsMode.Precise;
		PublicDependencyModuleNames.AddRange(new string[] { "Core" });

		string UeProjectRoot = Path.GetFullPath(Path.Combine(PluginDirectory, "..", ".."));   // ...\ue5
		string RepoRoot = Path.GetFullPath(Path.Combine(UeProjectRoot, ".."));                  // ...\KSP-UE5 (or the worktree)

		if (bUseRealCore)
		{
			string EnvRoot = Environment.GetEnvironmentVariable("KSPCORE_ROOT");
			string CoreRoot = string.IsNullOrEmpty(EnvRoot) ? Path.Combine(RepoRoot, "core") : EnvRoot;
			string EnvManifest = Environment.GetEnvironmentVariable("KSPCORE_LINK_MANIFEST");
			string ManifestFile = string.IsNullOrEmpty(EnvManifest)
				? Path.Combine(CoreRoot, "build", "RelWithDebInfo", "kspcore_link.txt") : EnvManifest;
			ApplyManifest(ManifestFile);
			PublicDefinitions.Add("KSPCORE_STUB=0");
		}
		else
		{
			string StubRoot = Path.Combine(UeProjectRoot, "Stub");
			string LibFile = Path.Combine(StubRoot, "lib", "kspcore.lib");
			if (!File.Exists(LibFile))
			{
				throw new BuildException("KspCoreBridge: stub kspcore.lib not found at '" + LibFile + "'. Run ue5/Scripts/build_stub.ps1.");
			}
			PublicSystemIncludePaths.Add(Path.Combine(StubRoot, "include"));
			PublicAdditionalLibraries.Add(LibFile);
			PublicDefinitions.Add("KSPCORE_STUB=1");
		}
	}

	private void ApplyManifest(string ManifestFile)
	{
		if (!File.Exists(ManifestFile))
		{
			throw new BuildException("KspCoreBridge: link manifest not found at '" + ManifestFile + "'. Build the core RelWithDebInfo first " +
				"(powershell core\\run_tests.ps1 -BuildOnly), or set KSPCORE_LINK_MANIFEST.");
		}

		int Version = 0;
		List<string> Includes = new List<string>();
		List<string> Libs = new List<string>();
		List<string> Defines = new List<string>();
		foreach (string RawLine in File.ReadAllLines(ManifestFile))
		{
			string Line = RawLine.Trim();
			if (Line.Length == 0 || Line.StartsWith("#"))
			{
				continue;
			}
			int Eq = Line.IndexOf('=');
			if (Eq <= 0)
			{
				throw new BuildException("KspCoreBridge: malformed line in '" + ManifestFile + "': " + Line);
			}
			string Key = Line.Substring(0, Eq);
			string Value = Line.Substring(Eq + 1).Trim();
			if (Value.Length == 0)
			{
				continue;   // an empty generator-expression result
			}
			switch (Key)
			{
				case "manifest_version": Version = int.Parse(Value); break;
				case "config": break;   // informational
				case "include": Includes.Add(Value); break;
				case "lib": Libs.Add(Value); break;
				case "define": Defines.Add(Value); break;
				default: throw new BuildException("KspCoreBridge: unknown manifest key '" + Key + "' in '" + ManifestFile + "'");
			}
		}

		if (Version < 1 || Version > SupportedManifestVersion)
		{
			throw new BuildException("KspCoreBridge: link manifest version " + Version + " is not supported (this reader handles 1.." +
				SupportedManifestVersion + "): " + ManifestFile);
		}
		if (Libs.Count == 0 || Includes.Count == 0)
		{
			throw new BuildException("KspCoreBridge: link manifest lists no libraries or no include directory: " + ManifestFile);
		}
		foreach (string Inc in Includes)
		{
			if (!Directory.Exists(Inc))
			{
				throw new BuildException("KspCoreBridge: include directory from the manifest does not exist: " + Inc);
			}
			PublicSystemIncludePaths.Add(Inc);
		}
		foreach (string Lib in Libs)
		{
			if (!File.Exists(Lib))
			{
				throw new BuildException("KspCoreBridge: library from the manifest does not exist: " + Lib + " (manifest " + ManifestFile + ")");
			}
			PublicAdditionalLibraries.Add(Lib);   // order = link order
		}
		// Definitions that must match what the libraries were built with. Only Jolt's configuration (JPH_*) matters; the bridge
		// does not include Jolt headers, so these exist for the consistency check in KspCoreBridge.cpp (the core reports the Jolt
		// it actually is through kspcore_test_jolt_config()). Config-dependent ones such as NDEBUG are not forwarded.
		foreach (string Def in Defines)
		{
			if (Def.StartsWith("JPH_"))
			{
				PrivateDefinitions.Add(Def);
			}
		}
		PrivateDefinitions.Add("KSPCORE_MANIFEST_LIB_COUNT=" + Libs.Count);
		// UBT re-links when the manifest-listed libraries change (it tracks external libs); make the manifest itself a dependency
		// too, so a changed library list or definition set rebuilds the bridge.
		ExternalDependencies.Add(ManifestFile);
	}
}
