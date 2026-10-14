using UnrealBuildTool;

public class SessionAnalyticsDemo : ModuleRules
{
	public SessionAnalyticsDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "SessionAnalytics" });
	}
}
