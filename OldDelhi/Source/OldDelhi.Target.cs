using UnrealBuildTool;

public class OldDelhiTarget : TargetRules
{
	public OldDelhiTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("OldDelhi");
	}
}
