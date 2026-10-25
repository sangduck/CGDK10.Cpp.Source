using UnrealBuildTool;

public class CgdkUeTarget : TargetRules
{
	public CgdkUeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("CgdkUe");
	}
}
