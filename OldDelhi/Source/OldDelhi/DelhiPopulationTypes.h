#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DelhiPopulationTypes.generated.h"

/** Row of DT_Archetypes.csv (research/05 section 2). */
USTRUCT(BlueprintType)
struct FDelhiArchetypeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SpawnGroup;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AgeMin = 18;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AgeMax = 60;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaleShare = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float FemaleShare = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float OtherShare = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString OutfitSummer;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString OutfitMonsoon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString OutfitWinter;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Accessories;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Schedule;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString BodyLanguage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Places;
	/** "Hi=50;Ur=10;..." */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Languages;
};

/** Row of DT_StreetDensity.csv: visible agents per 100 m of street by time block. */
USTRUCT(BlueprintType)
struct FDelhiStreetDensityRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) float H05 = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float H09 = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float H13 = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float H18 = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float H22 = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float H02 = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float StaticDressingFactor = 0.4f;
};

/** Row of DT_PopulationSchedule.csv: archetype share for a (time block, street type). */
USTRUCT(BlueprintType)
struct FDelhiScheduleRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName TimeBlock;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Street;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Archetype;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Weight = 0.f;
};

/** Row of DT_PopulationEvents.csv. */
USTRUCT(BlueprintType)
struct FDelhiPopulationEventRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Window;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Zones;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Multiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Effect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString AddedArchetypes;
};

/** Row of DT_ClimateMonth.csv (research/02 A4.3): daily weather-state probabilities + persistence. */
USTRUCT(BlueprintType)
struct FDelhiClimateMonthRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Clear = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Haze = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Overcast = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Rain = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Storm = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Fog = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Smog = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Persistence = 0.6f;
};
