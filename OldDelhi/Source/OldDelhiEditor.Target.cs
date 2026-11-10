using UnrealBuildTool;

public class OldDelhiEditorTarget : TargetRules
{
	public OldDelhiEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("OldDelhi");
	}
}
