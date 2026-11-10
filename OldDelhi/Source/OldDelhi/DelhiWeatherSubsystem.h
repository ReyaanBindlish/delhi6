#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DelhiWeatherSubsystem.generated.h"

class UMaterialParameterCollection;

/** Weather states driven from real Delhi climatology (see game-design-docs/research/02-weather-timeline-and-terrain.md). */
UENUM(BlueprintType)
enum class EDelhiWeather : uint8
{
	Clear,
	Haze,
	Smog,
	Fog,
	Rain,
	Monsoon,
	DustStorm
};

/**
 * Single owner of time-of-day and weather. Writes MPC_Weather so every material, Niagara system and
 * audio zone reads one source of truth (implementation.md section 3.7). Physics consumers sample
 * Wetness / Wind from here at their own rate (research/03-physics-micro-engine.md).
 */
UCLASS()
class OLDDELHI_API UDelhiWeatherSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDelhiWeatherSubsystem, STATGROUP_Tickables); }

	UFUNCTION(BlueprintCallable, Category = "Delhi|Weather")
	void SetWeather(EDelhiWeather NewWeather, float BlendSeconds = 30.f);

	UFUNCTION(BlueprintPure, Category = "Delhi|Weather")
	float GetWetness() const { return Wetness; }

	UFUNCTION(BlueprintPure, Category = "Delhi|Weather")
	FVector GetWind() const { return Wind; }

	/** Optional collection to write parameters into (Wetness, Dust, Smog, WindX, WindY, WindStrength). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	TObjectPtr<UMaterialParameterCollection> WeatherCollection;

	/** Game minutes elapsed per real second (research/02: 30 by day, 60 by night). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	float DayRate = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	float NightRate = 60.f;

	/** Hours since midnight, local time (IST). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delhi|Time")
	float TimeOfDayHours = 16.f;

private:
	EDelhiWeather Current = EDelhiWeather::Haze;
	EDelhiWeather Target = EDelhiWeather::Haze;
	float Blend = 1.f;
	float BlendRate = 1.f;

	float Wetness = 0.f;
	float Dust = 0.f;
	float Smog = 0.4f;
	FVector Wind = FVector(2.f, 0.f, 0.f);
};
