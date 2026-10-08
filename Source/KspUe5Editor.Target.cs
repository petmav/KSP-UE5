using UnrealBuildTool;
using System.Collections.Generic;

public class KspUe5EditorTarget : TargetRules
{
	public KspUe5EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "KspUe5", "KspUe5Editor" });
	}
}
