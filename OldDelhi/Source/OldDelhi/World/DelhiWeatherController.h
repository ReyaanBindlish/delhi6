#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/DelhiAtmosphereTypes.h"
#include "DelhiWeatherController.generated.h"

class UMaterialParameterCollection;
class UDelhiWeatherSubsystem;

/**
 * Level-placed configuration for UDelhiWeatherSubsystem, plus the Blueprint-facing hooks that rain/dust Niagara,
 * audio zones and physics assets bind to. Put one in each level (setup_lighting.py does).
 */
UCLASS()
class OLDDELHI_API ADelhiWeatherController : public AActor
{
	GENERATED_BODY()

public:
	ADelhiWeatherController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ---------------------------------------------------------------- authoring
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	EAtmosphereCity City = EAtmosphereCity::Delhi;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	bool bAutoWeather = true;

	/** When on, the weather starts as InitialWeather and stays there until ReleaseOverride (good for the four Phase 1 test days). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	bool bForceInitialWeather = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	EDelhiWeather InitialWeather = EDelhiWeather::Haze;

	/** 0 = random. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	int32 RandomSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	TObjectPtr<UMaterialParameterCollection> WeatherCollection;

	// ---------------------------------------------------------------- time
	/** Apply StartYear/Month/Day/Hour to the story calendar at BeginPlay (leave off when the save game sets the date). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	bool bApplyStartTime = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	int32 StartYear = 2019;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time", meta = (ClampMin = "1", ClampMax = "12"))
	int32 StartMonth = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time", meta = (ClampMin = "1", ClampMax = "31"))
	int32 StartDay = 14;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time", meta = (ClampMin = "0", ClampMax = "23.99"))
	float StartHour = 16.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	float DayRate = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	float NightRate = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	float TimeScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	bool bPaused = false;

	// ---------------------------------------------------------------- runtime helpers (Blueprint)
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") UDelhiWeatherSubsystem* GetWeatherSubsystem() const;
	UFUNCTION(BlueprintCallable, Category = "Delhi|Weather") void ForceWeather(EDelhiWeather Weather, float BlendSeconds = 30.f);
	UFUNCTION(BlueprintCallable, Category = "Delhi|Weather") void ReleaseOverride();
	UFUNCTION(BlueprintCallable, Category = "Delhi|Time") void SetTimeOfDay(float Hours);

	/** 0 dry .. 1 soaked; use for puddle decals, tyre grip, footstep sets. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetWetness() const;
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetRainIntensity() const;
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetTractionMultiplier() const;
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") FVector GetWind() const;

	/** Fires when the weather state changes (automatic or forced). Drive Niagara rain/dust and audio zones from here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Delhi|Weather")
	void OnWeatherStateChanged(EDelhiWeather NewWeather, EDelhiWeather PreviousWeather);

	/** Fires at the first tick of each new day with that day's dominant state. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Delhi|Weather")
	void OnDailyStateRolled(EDelhiDailyState DailyState);

private:
	UFUNCTION() void HandleWeatherChanged(EDelhiWeather NewWeather, EDelhiWeather PreviousWeather);
	UFUNCTION() void HandleDailyRolled(EDelhiDailyState DailyState);
};
