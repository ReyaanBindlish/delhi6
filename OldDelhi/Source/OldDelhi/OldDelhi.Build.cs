using UnrealBuildTool;

public class OldDelhi : ModuleRules
{
	public OldDelhi(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "PhysicsCore"
		});
	}
}
