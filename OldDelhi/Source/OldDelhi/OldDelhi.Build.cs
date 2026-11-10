using UnrealBuildTool;

public class OldDelhi : ModuleRules
{
	public OldDelhi(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Source files include each other by module-root-relative paths (e.g. "Player/DayaCharacter.h").
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "PhysicsCore",
			"EnhancedInput",
			"DeveloperSettings",
			"Json",
			"ChaosVehicles", "ChaosVehiclesCore",
			"MassEntity", "MassCore", "MassCommon", "MassSpawner", "MassMovement", "MassSimulation"
		});
	}
}
