#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Physics/DelhiSurfaceTypes.h"
#include "DelhiSurfaceLibrary.generated.h"

/** Surface lookup and wetness-aware friction (reads UDelhiWeatherSubsystem::GetWetness). */
UCLASS()
class OLDDELHI_API UDelhiSurfaceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Static table access (C++). */
	static const FDelhiSurfaceData& Lookup(EDelhiSurface Surface);

	UFUNCTION(BlueprintPure, Category = "Delhi|Physics")
	static FDelhiSurfaceData GetSurfaceData(EDelhiSurface Surface);

	UFUNCTION(BlueprintPure, Category = "Delhi|Physics")
	static EDelhiSurface FromPhysicalSurface(EPhysicalSurface PhysicalSurface);

	UFUNCTION(BlueprintPure, Category = "Delhi|Physics")
	static EPhysicalSurface ToPhysicalSurface(EDelhiSurface Surface);

	/** Needs the trace to have been run with bReturnPhysicalMaterial = true. Default surface if none. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Physics")
	static EDelhiSurface GetSurfaceFromHit(const FHitResult& Hit);

	/** Friction multiplier from weather for a surface (1 = dry). WetnessOverride < 0 reads the weather subsystem. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Physics", meta = (WorldContext = "WorldContextObject"))
	static float GetWetFrictionScale(const UObject* WorldContextObject, EDelhiSurface Surface, float WetnessOverride = -1.f);

	/** Dry friction * wetness scale. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Physics", meta = (WorldContext = "WorldContextObject"))
	static float GetEffectiveFriction(const UObject* WorldContextObject, EDelhiSurface Surface, float WetnessOverride = -1.f);

	/** Current wetness 0-1 from the weather subsystem (0 if unavailable). */
	UFUNCTION(BlueprintPure, Category = "Delhi|Physics", meta = (WorldContext = "WorldContextObject"))
	static float GetCurrentWetness(const UObject* WorldContextObject);
};
