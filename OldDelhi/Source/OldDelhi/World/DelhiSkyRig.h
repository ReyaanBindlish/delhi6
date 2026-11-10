#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/DelhiAtmosphereTypes.h"
#include "DelhiSkyRig.generated.h"

class USceneComponent;
class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UVolumetricCloudComponent;
class UPostProcessComponent;
class UMaterialInstanceDynamic;
class UDelhiWeatherSubsystem;

/** How the sun light's lux is chosen. */
UENUM(BlueprintType)
enum class ESunIntensityMode : uint8
{
	/** Top-of-atmosphere 127,500 lux times the cloud beam factor; the Sky Atmosphere does the Beer-Lambert attenuation (no double counting). */
	AtmosphereAttenuated,
	/** Use the research/02 B2.6 table value (E_dn) directly. For calibration or when the atmosphere's transmittance is disabled. */
	ResearchTable
};

/**
 * One actor that is the whole sky (BP_SkyRig in research/02 section C): sun + moon directional lights driven by a real
 * NOAA sun position, SkyAtmosphere, real-time SkyLight, ExponentialHeightFog with volumetric fog, VolumetricCloud and an
 * unbound physical-exposure PostProcess.
 *
 * In game it follows UDelhiWeatherSubsystem (clock from UStoryCalendarSubsystem, weather from the Markov chain).
 * In the editor it shows the Preview* date/time and weather so lighting can be authored for a chosen city and moment.
 * Place it at ground level: the height fog sits at the actor's Z. Only one rig (one SkyAtmosphere) per level.
 */
UCLASS()
class OLDDELHI_API ADelhiSkyRig : public AActor
{
	GENERATED_BODY()

public:
	ADelhiSkyRig();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Re-evaluate everything now (also runs from the details panel). */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Delhi|Sky")
	void RefreshSky();

	UFUNCTION(BlueprintPure, Category = "Delhi|Sky") float GetSunElevationDeg() const { return SunElevationDeg; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Sky") float GetSunAzimuthDeg() const { return SunAzimuthDeg; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Sky") float GetTargetEV100() const { return TargetEV100; }
	UFUNCTION(BlueprintPure, Category = "Delhi|Sky") float GetSunLux() const { return SunLux; }

	// ---------------------------------------------------------------- components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delhi|Sky") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delhi|Sky") TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delhi|Sky") TObjectPtr<UDirectionalLightComponent> Moon;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delhi|Sky") TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delhi|Sky") TObjectPtr<USkyLightComponent> SkyLightComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delhi|Sky") TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delhi|Sky") TObjectPtr<UVolumetricCloudComponent> Clouds;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delhi|Sky") TObjectPtr<UPostProcessComponent> PostProcess;

	// ---------------------------------------------------------------- location
	/** In game the city comes from UDelhiWeatherSubsystem; untick to use City below. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Location")
	bool bFollowWeatherSubsystem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Location")
	EAtmosphereCity City = EAtmosphereCity::Delhi;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Location")
	bool bUseCustomGeo = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Location", meta = (EditCondition = "bUseCustomGeo"))
	FAtmosphereCityGeo CustomGeo;

	/** Unreal yaw that points at true north. This project is X east, Y south => -90. Keep equal to the weather subsystem's value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Location")
	float NorthYawDegrees = -90.f;

	// ---------------------------------------------------------------- editor preview (ignored in game when following the subsystem)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Preview")
	int32 PreviewYear = 2019;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Preview", meta = (ClampMin = "1", ClampMax = "12"))
	int32 PreviewMonth = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Preview", meta = (ClampMin = "1", ClampMax = "31"))
	int32 PreviewDay = 14;
	/** Local hours, 0-24. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Preview", meta = (ClampMin = "0", ClampMax = "23.99"))
	float PreviewHour = 16.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Preview")
	EDelhiWeather PreviewWeather = EDelhiWeather::Haze;

	// ---------------------------------------------------------------- tuning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	ESunIntensityMode SunMode = ESunIntensityMode::AtmosphereAttenuated;

	/** Multiplier on the sun lux for calibration against reference photos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning", meta = (ClampMin = "0"))
	float SunIlluminanceScale = 1.f;

	/** Top-of-atmosphere sun illuminance in lux (research/02 B2.6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	float SunTopOfAtmosphereLux = 127500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	bool bEnableMoon = true;

	/** Full-moon illuminance in lux (about 0.1-0.3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	float FullMoonLux = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	bool bControlExposure = true;

	/** Added to the computed EV100 target (positive = darker image). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	float ExposureBias = 0.f;

	/** EV100 anchor for night lanes lit by LED street lights (research: 2-4; Diwali market 4-5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	float NightEV100 = 3.5f;

	/** Exposure band around the target: Min = target - this, Max = target + MaxAbove, clamped to [1,15] (B2.6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	float ExposureBelow = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	float ExposureAbove = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	bool bControlFog = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	bool bControlAtmosphere = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	bool bControlClouds = true;

	/** Scalar parameter on the cloud material that sets coverage. Missing parameters are ignored; set to None to leave the cloud material alone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning")
	FName CloudCoverageParameter = TEXT("Coverage");

	/** Multiplier on the fog inscattering luminance (physical exposure makes fixed luminances read dark or bright; tune here). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning", meta = (ClampMin = "0"))
	float FogInscatterScale = 1.f;

	/** Seconds between heavy updates (atmosphere, fog, post). Sun rotation updates every frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delhi|Tuning", meta = (ClampMin = "0.02"))
	float UpdateIntervalSeconds = 0.2f;

private:
	FDateTime ResolveTime(bool bGame, UDelhiWeatherSubsystem* Weather) const;
	void Apply(float DeltaSeconds, bool bForce);

	float SunElevationDeg = 30.f;
	float SunAzimuthDeg = 180.f;
	float TargetEV100 = 13.f;
	float SunLux = 0.f;
	float HeavyTimer = 0.f;
	bool bSunShadows = true;

	float BaseRayleigh = 1.f;
	float BaseMieScattering = 1.f;
	float BaseMieAbsorption = 1.f;
	float LastAtmosphereKey = -1.f;

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> CloudMID;
};
