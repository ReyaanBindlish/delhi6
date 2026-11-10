#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DelhiPopulationTypes.h"
#include "DelhiPopulationGovernor.generated.h"

class UDataTable;

/**
 * Population governor (research/05 section 5): turns time of day, weather and events into a target
 * visible-agent density per street type, and picks archetypes by weighted schedule. Mass spawners
 * converge to these targets over 60-120 s; this class only decides "how many and who".
 */
UCLASS()
class OLDDELHI_API UDelhiPopulationGovernor : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Population") TObjectPtr<UDataTable> StreetDensity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Population") TObjectPtr<UDataTable> Schedule;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Population") TObjectPtr<UDataTable> Archetypes;

	/** Hard caps from research/05 section 1.5. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Population") int32 VisibleCap = 600;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Population") int32 EventVisibleCap = 750;
	/** Player setting: Low 0.5, Medium 0.75, High 1.0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Population") float CrowdDensitySetting = 1.f;

	/** Weather multiplier on pedestrians (rain 0.55, smog 0.8, heat-midday 0.6, fog-morning 0.7). */
	UPROPERTY(BlueprintReadWrite, Category = "Delhi|Population") float WeatherMultiplier = 1.f;
	/** Active event multiplier (Diwali shopping 1.6, Friday prayers at mosque 40+, ...). */
	UPROPERTY(BlueprintReadWrite, Category = "Delhi|Population") float EventMultiplier = 1.f;

	/** Visible agents per 100 m for a street type (MainRoad, BazaarGalli, ...) at a time (hours 0-24). */
	UFUNCTION(BlueprintCallable, Category = "Delhi|Population")
	float GetTargetDensity(FName Street, float Hours) const;

	/** Number of visible agents to aim for on a street segment of the given length (m). */
	UFUNCTION(BlueprintCallable, Category = "Delhi|Population")
	int32 GetTargetCount(FName Street, float Hours, float SegmentLengthM, bool bEventActive = false) const;

	/** Weighted pick of an archetype id (A01..A52, STATIC_SLEEPER, MIXED) for a street at a time. */
	UFUNCTION(BlueprintCallable, Category = "Delhi|Population")
	FName PickArchetype(FName Street, float Hours, int32 Seed) const;

private:
	static void BlockFor(float Hours, FName& A, FName& B, float& Alpha);
	float DensityAtBlock(const FDelhiStreetDensityRow& Row, FName Block) const;
};
