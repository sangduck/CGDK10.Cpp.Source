using UnrealBuildTool;

public class CgdkUeEditorTarget : TargetRules
{
	public CgdkUeEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("CgdkUe");
	}
}
