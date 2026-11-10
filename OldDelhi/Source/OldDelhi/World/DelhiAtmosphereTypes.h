#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DelhiAtmosphereTypes.generated.h"

struct FDelhiClimateMonthRow;

/**
 * Weather states (research/02 A4.3 + B2.7). The first seven values predate this file and keep their order;
 * new values are appended so existing assets and Blueprints stay valid.
 */
UENUM(BlueprintType)
enum class EDelhiWeather : uint8
{
	Clear,
	Haze,
	Smog,
	Fog,
	Rain,
	Monsoon,
	DustStorm,
	Overcast,
	Thunderstorm,
	/** Karachi humid marine haze (sea-breeze afternoons). */
	SeaHaze
};

/** Story cities that have a real sun position and a climatology. */
UENUM(BlueprintType)
enum class EAtmosphereCity : uint8
{
	Delhi,
	Karachi
};

/** Daily dominant state: the columns of DT_ClimateMonth.csv. */
UENUM(BlueprintType)
enum class EDelhiDailyState : uint8
{
	Clear,
	Haze,
	Overcast,
	Rain,
	Storm,
	Fog,
	Smog
};

/** Where a city is on Earth. Time zone is explicit (research/02 A4.2: never trust a default). */
USTRUCT(BlueprintType)
struct FAtmosphereCityGeo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name = TEXT("Delhi");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Latitude = 28.656f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Longitude = 77.232f;
	/** Hours east of UTC (IST 5.5, PKT 5.0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float UtcOffsetHours = 5.5f;
};

/**
 * Everything the sky, fog, post-process and MPC need for one weather state. Scales are relative to the
 * Sky Atmosphere component's own defaults (research/02 B2.7 "scale relative to default 1.0").
 */
USTRUCT(BlueprintType)
struct FDelhiAtmosphereState
{
	GENERATED_BODY()

	// Sky Atmosphere (relative to component defaults)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RayleighScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MieScatteringScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MieAbsorptionScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MieAnisotropy = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MieScaleHeightKm = 1.2f;

	// Exponential height fog
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FogDensity = 0.01f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FogHeightFalloff = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor FogInscattering = FLinearColor(0.45f, 0.55f, 0.75f, 1.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float VolumetricFogExtinctionScale = 1.f;

	// Sun
	/** Aerosol optical depth at 550 nm (research/02 B2.6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AOD = 0.7f;
	/** 0..1 effective cloud cover for the direct beam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CloudCover = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SunTemperatureK = 5900.f;
	/** Sky/diffuse horizontal illuminance at noon, lux (B2.6 table). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DiffuseNoonLux = 30000.f;
	/** Suggested EV100 for bright scene areas at noon (B2.6 table). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float BaseEV100 = 14.5f;

	// Surface / physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Wetness = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Dust = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Smog = 0.15f;
	/** m/s at street level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WindStrength = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RainIntensity = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float VisibilityMeters = 20000.f;

	// Grade
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Saturation = 1.f;

	static FDelhiAtmosphereState Lerp(const FDelhiAtmosphereState& A, const FDelhiAtmosphereState& B, float T);
};

/** Pure functions: real sun and moon, climatology, atmosphere presets, exposure. */
UCLASS()
class OLDDELHI_API UDelhiAtmosphereLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Delhi|Atmosphere")
	static FAtmosphereCityGeo GetCityGeo(EAtmosphereCity City);

	/** Preset for one weather state in one city (research/02 B2.6, B2.7). */
	UFUNCTION(BlueprintPure, Category = "Delhi|Atmosphere")
	static FDelhiAtmosphereState GetBaseState(EDelhiWeather Weather, EAtmosphereCity City);

	/**
	 * NOAA solar position. LocalTime is wall-clock time in the city's zone (what UStoryCalendarSubsystem holds).
	 * Azimuth is degrees clockwise from true north; elevation includes atmospheric refraction when requested.
	 */
	UFUNCTION(BlueprintPure, Category = "Delhi|Atmosphere")
	static void ComputeSunPosition(const FDateTime& LocalTime, float LatitudeDeg, float LongitudeDeg, float UtcOffsetHours,
	                               float& OutAzimuthDeg, float& OutElevationDeg, bool bApplyRefraction = true);

	/** Moon position approximated from the synodic age (good to a few degrees; no lunar inclination). */
	UFUNCTION(BlueprintPure, Category = "Delhi|Atmosphere")
	static void ComputeMoon(const FDateTime& LocalTime, float LatitudeDeg, float LongitudeDeg, float UtcOffsetHours,
	                        float& OutAzimuthDeg, float& OutElevationDeg, float& OutIlluminatedFraction);

	/**
	 * Rotation for a directional light that represents a body at the given azimuth/elevation.
	 * NorthYawDegrees is the Unreal yaw that points at true north. This project: X east, Y south => -90.
	 */
	UFUNCTION(BlueprintPure, Category = "Delhi|Atmosphere")
	static FRotator LightRotationFromSun(float AzimuthDeg, float ElevationDeg, float NorthYawDegrees = -90.f);

	/** Direct-normal sun illuminance by Beer-Lambert (research/02 B2.6), including the cloud factor. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Atmosphere")
	static float EstimateDirectSunLux(float ElevationDeg, float AOD, float CloudCover);

	/** Cloud transmittance applied to the direct beam. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Atmosphere")
	static float CloudBeamFactor(float CloudCover);

	/** Target EV100 for the exposure band at this sun elevation (day from the B2.6 table, night anchored at NightEV100). */
	UFUNCTION(BlueprintPure, Category = "Delhi|Atmosphere")
	static float EstimateExposureEV100(const FDelhiAtmosphereState& State, float SunElevationDeg, float NightEV100 = 3.5f);

	/** 0 at night, 1 in daylight; smooth through civil twilight. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Atmosphere")
	static float DayFactor(float SunElevationDeg);

	/** Climate row from the built-in tables (Delhi mirrors DT_ClimateMonth.csv; Karachi is an estimate [E]). Month 1..12. */
	static FDelhiClimateMonthRow GetBuiltinClimate(EAtmosphereCity City, int32 Month);

	/** Daily state + hour -> concrete weather, with the city's character (Karachi swaps fog/smog for sea haze). */
	static EDelhiWeather ResolveDailyWeather(EDelhiDailyState Daily, EAtmosphereCity City, int32 Month, bool bInEpisode, float Hour, float FogBurnOffHour);

	/** Prevailing wind: compass bearing the wind blows FROM. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Atmosphere")
	static float PrevailingWindFromBearing(EAtmosphereCity City, int32 Month, EDelhiWeather Weather);
};
