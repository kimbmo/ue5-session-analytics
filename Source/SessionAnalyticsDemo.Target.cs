using UnrealBuildTool;

public class SessionAnalyticsDemoTarget : TargetRules
{
	public SessionAnalyticsDemoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SessionAnalyticsDemo");
	}
}
