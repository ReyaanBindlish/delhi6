#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/DelhiAtmosphereTypes.h"
#include "DelhiWeatherSubsystem.generated.h"

class UMaterialParameterCollection;
class UDataTable;
class UStoryCalendarSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDelhiWeatherChanged, EDelhiWeather, NewWeather, EDelhiWeather, PreviousWeather);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDelhiDailyWeatherRolled, EDelhiDailyState, DailyState);

/**
 * Single owner of time-of-day and weather (implementation.md 3.7). EDelhiWeather and the presets live in
 * World/DelhiAtmosphereTypes.h.
 *
 *  - Clock: runs on UStoryCalendarSubsystem's date (research/02 A4.2: 30 game s per real s by day, 60 at night,
 *    smoothed through dawn and dusk). The calendar DATE only changes by story beats unless bClockRollsDate.
 *  - Weather: per-day Markov chain over DT_ClimateMonth (Delhi) or a built-in Karachi table, with rain/storm
 *    episodes and fog burn-off inside the day. SetWeather() is a manual override.
 *  - Output: MPC_Weather (Wetness, Dust, Smog, Wind*, RainIntensity, Visibility, SunElevation, NightFactor, TimeOfDay)
 *    and BlueprintPure getters for physics (wetness, traction, wind).
 */
UCLASS()
class OLDDELHI_API UDelhiWeatherSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDelhiWeatherSubsystem, STATGROUP_Tickables); }

	// ---------------------------------------------------------------- weather control
	/** Manual override: blends to this weather and stops the Markov chain until ReleaseWeatherOverride(). */
	UFUNCTION(BlueprintCallable, Category = "Delhi|Weather")
	void SetWeather(EDelhiWeather NewWeather, float BlendSeconds = 30.f);

	UFUNCTION(BlueprintCallable, Category = "Delhi|Weather")
	void ReleaseWeatherOverride();

	/** Throw the dice for today again (uses the stream; keeps the same date). */
	UFUNCTION(BlueprintCallable, Category = "Delhi|Weather")
	void RerollDailyWeather();

	UFUNCTION(BlueprintCallable, Category = "Delhi|Weather")
	void SetCity(EAtmosphereCity NewCity);

	/** Re-seed the weather dice (0 = random). */
	UFUNCTION(BlueprintCallable, Category = "Delhi|Weather")
	void SetRandomSeed(int32 NewSeed);

	// ---------------------------------------------------------------- time control
	UFUNCTION(BlueprintCallable, Category = "Delhi|Time")
	void SetTimeOfDay(float Hours);

	/** Story sleep / skip: advances the calendar (fires OnDayEnded for midnights crossed) and re-resolves weather. */
	UFUNCTION(BlueprintCallable, Category = "Delhi|Time")
	void SkipHours(float Hours);

	UFUNCTION(BlueprintPure, Category = "Delhi|Time")
	FDateTime GetLocalDateTime() const;

	// ---------------------------------------------------------------- queries
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") EDelhiWeather GetWeather() const { return Target; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") EDelhiDailyState GetDailyState() const { return DailyState; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") EAtmosphereCity GetCity() const { return City; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetWetness() const { return Wetness; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetDust() const { return State.Dust; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetSmog() const { return State.Smog; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetRainIntensity() const { return State.RainIntensity; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetVisibilityMeters() const { return State.VisibilityMeters; }
	/** World-space wind velocity in m/s (direction the air moves), with gusts. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") FVector GetWind() const { return Wind; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetWindSpeed() const { return Wind.Size(); }
	/** Wind at a place: same vector with a slow spatial variation so cloth, smoke and leaves do not move in lockstep. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") FVector GetWindAtLocation(const FVector& WorldLocation) const;
	/** Tyre/foot traction multiplier from surface water and loose dust (1 dry, about 0.55 soaked). Physics consumers multiply friction by this. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetTractionMultiplier() const;
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") FDelhiAtmosphereState GetAtmosphereState() const { return State; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetSunElevationDeg() const { return SunElevation; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Weather") float GetNightFactor() const { return NightFactor; }

	/** The sky rig reports its sun here so MPC_Weather carries it for street lights, emissives and audio. */
	void ReportSun(float ElevationDeg, float AzimuthDeg);

	// ---------------------------------------------------------------- events
	UPROPERTY(BlueprintAssignable, Category = "Delhi|Weather") FOnDelhiWeatherChanged OnWeatherChanged;
	UPROPERTY(BlueprintAssignable, Category = "Delhi|Weather") FOnDelhiDailyWeatherRolled OnDailyWeatherRolled;

	// ---------------------------------------------------------------- config
	/** Collection to write into (auto-loaded from /Game/Delhi/Materials/MPC_Weather when empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	TObjectPtr<UMaterialParameterCollection> WeatherCollection;

	/** Row struct FDelhiClimateMonthRow, row names Jan..Dec (DT_ClimateMonth.csv). Empty = built-in copy of the CSV. Used for Delhi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	TObjectPtr<UDataTable> ClimateTable;

	/** Optional Karachi table, same row struct. Empty = built-in estimate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	TObjectPtr<UDataTable> KarachiClimateTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	EAtmosphereCity City = EAtmosphereCity::Delhi;

	/** Markov daily weather on/off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	bool bAutoWeather = true;

	/** 0 = random each session. Set for reproducible weather (tests, replays). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	int32 RandomSeed = 0;

	/** Real seconds for an automatic transition (about 10 game minutes by day). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather", meta = (ClampMin = "1"))
	float AutoBlendSeconds = 45.f;

	/** Rain episode length in game hours (research/02 A4.4: states dwell at least ~10 real minutes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	FVector2D RainEpisodeHours = FVector2D(3.5, 8.0);

	/** Dust storm / thunderstorm episode length in game hours (storms are short). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	FVector2D StormEpisodeHours = FVector2D(0.8, 2.2);

	/** Unreal yaw that points at true north; this project has X east, Y south, so -90. Must match the sky rig. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Weather")
	float NorthYawDegrees = -90.f;

	// ---------------------------------------------------------------- clock config
	/** Game seconds per real second by day / by night (research/02 A4.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	float DayRate = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	float NightRate = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	float TimeScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	bool bPaused = false;

	/** Write the running clock into UStoryCalendarSubsystem. Off = clock frozen, story beats move time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	bool bDriveStoryCalendar = true;

	/** Midnight rolls the calendar date. Default off: the date moves by story beats (A4.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	bool bClockRollsDate = false;

	/** Clock used when there is no game instance (editor preview worlds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Time")
	FDateTime FallbackDate = FDateTime(2019, 10, 14, 16, 0, 0);

	/** Hours since midnight, local time. Mirrors the calendar. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delhi|Time")
	float TimeOfDayHours = 16.f;

private:
	UStoryCalendarSubsystem* GetCalendar() const;
	const UDataTable* GetClimateTable() const;
	void StepClock(float DeltaTime);
	void RollDay(const FDateTime& Date, bool bAllowPersistence);
	void SetLocalDateTime(const FDateTime& NewDate);
	void ResolveAndApplyWeather(const FDateTime& Date, float BlendSeconds);
	void BeginBlend(EDelhiWeather NewWeather, float BlendSeconds);
	void WriteCollection();
	float ClockRate(float Hour) const;

	EDelhiWeather Current = EDelhiWeather::Haze;
	EDelhiWeather Target = EDelhiWeather::Haze;
	float Blend = 1.f;
	float BlendRate = 1.f;
	FDelhiAtmosphereState FromState;
	FDelhiAtmosphereState ToState;
	FDelhiAtmosphereState State;

	bool bManualOverride = false;
	bool bHasRolled = false;
	FDateTime LastRolledDate;
	EDelhiDailyState DailyState = EDelhiDailyState::Haze;
	bool bHasEpisode = false;
	float EpisodeStartHour = 0.f;
	float EpisodeEndHour = 0.f;
	float FogBurnOffHour = 10.5f;

	FRandomStream Stream;
	float Wetness = 0.f;
	FVector Wind = FVector(2.f, 0.f, 0.f);
	float WindBearingFrom = 315.f;
	float SunElevation = 30.f;
	float SunAzimuth = 180.f;
	float NightFactor = 0.f;
	float NoiseTime = 0.f;
	float CollectionTimer = 0.f;
	bool bCollectionSearched = false;
};
