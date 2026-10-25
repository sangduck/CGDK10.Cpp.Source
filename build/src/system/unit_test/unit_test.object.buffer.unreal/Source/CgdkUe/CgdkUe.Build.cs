using System.IO;
using UnrealBuildTool;

public class CgdkUe : ModuleRules
{
	public CgdkUe(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		bEnableExceptions = true;
		CppStandard = CppStandardVersion.Cpp20;
		bUseUnity = false;

		// CGDK10.Cpp/include (CGDK_INCLUDE 환경 변수로 바꿀 수 있다.)
		string CgdkInclude = System.Environment.GetEnvironmentVariable("CGDK_INCLUDE") ?? Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../../../../include"));
		PublicSystemIncludePaths.Add(CgdkInclude);
	}
}
