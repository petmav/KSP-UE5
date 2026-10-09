using UnrealBuildTool;
using System.Collections.Generic;

public class KspUe5Target : TargetRules
{
	public KspUe5Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("KspUe5");
	}
}
