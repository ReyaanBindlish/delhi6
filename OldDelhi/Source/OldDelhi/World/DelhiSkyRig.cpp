#include "World/DelhiSkyRig.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "DelhiWeatherSubsystem.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

ADelhiSkyRig::ADelhiSkyRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->bAtmosphereSunLight = true;
	Sun->AtmosphereSunLightIndex = 0;
	Sun->LightSourceAngle = 0.53f;
	Sun->DynamicShadowDistanceMovableLight = 30000.f;
	Sun->SetIntensity(100000.f);

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(Root);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->bAtmosphereSunLight = true;
	Moon->AtmosphereSunLightIndex = 1;
	Moon->LightSourceAngle = 0.5f;
	Moon->SetIntensity(0.f);
	Moon->SetVisibility(false);

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	Atmosphere->SetupAttachment(Root);
	BaseRayleigh = Atmosphere->RayleighScatteringScale;
	BaseMieScattering = Atmosphere->MieScatteringScale;
	BaseMieAbsorption = Atmosphere->MieAbsorptionScale;

	SkyLightComp = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLightComp->SetupAttachment(Root);
	SkyLightComp->SetMobility(EComponentMobility::Movable);
	SkyLightComp->bRealTimeCapture = true;
	SkyLightComp->bLowerHemisphereIsBlack = false;

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	Fog->SetupAttachment(Root);

	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("VolumetricCloud"));
	Clouds->SetupAttachment(Root);

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(Root);
	PostProcess->bUnbound = true;
	PostProcess->BlendWeight = 1.f;
	PostProcess->Priority = 0.f;
}

void ADelhiSkyRig::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Editor / spawn preview. In game BeginPlay and Tick take over.
	Apply(0.f, true);
}

void ADelhiSkyRig::BeginPlay()
{
	Super::BeginPlay();
	CloudMID = nullptr;
	Apply(0.f, true);
}

void ADelhiSkyRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Apply(DeltaSeconds, false);
}

void ADelhiSkyRig::RefreshSky()
{
	Apply(0.f, true);
}

FDateTime ADelhiSkyRig::ResolveTime(bool bGame, UDelhiWeatherSubsystem* Weather) const
{
	if (bGame && Weather) { return Weather->GetLocalDateTime(); }
	const int32 Y = FMath::Clamp(PreviewYear, 1901, 2200);
	const int32 M = FMath::Clamp(PreviewMonth, 1, 12);
	const int32 D = FMath::Clamp(PreviewDay, 1, FDateTime::DaysInMonth(Y, M));
	return FDateTime(Y, M, D) + FTimespan::FromHours(FMath::Clamp(PreviewHour, 0.f, 23.99f));
}

void ADelhiSkyRig::Apply(float DeltaSeconds, bool bForce)
{
	UWorld* World = GetWorld();
	if (!World) { return; }
	const bool bGame = World->IsGameWorld();
	UDelhiWeatherSubsystem* Weather = (bGame && bFollowWeatherSubsystem) ? World->GetSubsystem<UDelhiWeatherSubsystem>() : nullptr;

	// ---------------------------------------------------------------- where and when
	const EAtmosphereCity UseCity = Weather ? Weather->GetCity() : City;
	const FAtmosphereCityGeo Geo = bUseCustomGeo ? CustomGeo : UDelhiAtmosphereLibrary::GetCityGeo(UseCity);
	const FDateTime Local = ResolveTime(bGame, Weather);

	float Az = 0.f, El = 0.f;
	UDelhiAtmosphereLibrary::ComputeSunPosition(Local, Geo.Latitude, Geo.Longitude, Geo.UtcOffsetHours, Az, El, true);
	SunAzimuthDeg = Az;
	SunElevationDeg = El;
	if (Weather) { Weather->ReportSun(El, Az); }

	const FDelhiAtmosphereState S = Weather ? Weather->GetAtmosphereState() : UDelhiAtmosphereLibrary::GetBaseState(PreviewWeather, UseCity);

	// ---------------------------------------------------------------- sun (every frame: rotation is cheap)
	Sun->SetWorldRotation(UDelhiAtmosphereLibrary::LightRotationFromSun(Az, El, NorthYawDegrees));

	float Lux;
	if (SunMode == ESunIntensityMode::ResearchTable)
	{
		Lux = UDelhiAtmosphereLibrary::EstimateDirectSunLux(El, S.AOD, S.CloudCover);
	}
	else
	{
		// The sky atmosphere attenuates by its own Beer-Lambert; keep the light through twilight so the sky glows,
		// and fade it only once the sun is more than ~18 degrees down (astronomical twilight).
		Lux = SunTopOfAtmosphereLux * UDelhiAtmosphereLibrary::CloudBeamFactor(S.CloudCover) * FMath::SmoothStep(-19.f, -16.f, El);
	}
	SunLux = Lux * SunIlluminanceScale;

	HeavyTimer += DeltaSeconds;
	const bool bHeavy = bForce || HeavyTimer >= UpdateIntervalSeconds;
	if (!bHeavy)
	{
		// Light-weight path: sun only.
		Sun->SetIntensity(SunLux);
		return;
	}
	HeavyTimer = 0.f;

	Sun->SetIntensity(SunLux);
	Sun->SetUseTemperature(true);
	Sun->SetTemperature(S.SunTemperatureK);
	Sun->SetVisibility(El > -19.f);
	const bool bWantShadows = El > -1.f;
	if (bWantShadows != bSunShadows || bForce)
	{
		bSunShadows = bWantShadows;
		Sun->SetCastShadows(bWantShadows);
	}

	// ---------------------------------------------------------------- moon
	if (bEnableMoon)
	{
		float MAz = 0.f, MEl = 0.f, Illum = 0.f;
		UDelhiAtmosphereLibrary::ComputeMoon(Local, Geo.Latitude, Geo.Longitude, Geo.UtcOffsetHours, MAz, MEl, Illum);
		const float Up = FMath::SmoothStep(-1.f, 6.f, MEl) * (1.f - FMath::SmoothStep(-2.f, 8.f, El)) * Illum;
		Moon->SetWorldRotation(UDelhiAtmosphereLibrary::LightRotationFromSun(MAz, MEl, NorthYawDegrees));
		Moon->SetIntensity(FullMoonLux * Up * (1.f - 0.8f * S.CloudCover));
		Moon->SetTemperature(4100.f);
		Moon->SetUseTemperature(true);
		Moon->SetVisibility(Up > 0.001f);
	}
	else
	{
		Moon->SetVisibility(false);
	}

	// ---------------------------------------------------------------- sky atmosphere (re-created only when values move)
	if (bControlAtmosphere)
	{
		const float Key = S.RayleighScale * 3.1f + S.MieScatteringScale * 1.7f + S.MieAbsorptionScale * 5.3f + S.MieAnisotropy * 7.9f + S.MieScaleHeightKm * 11.3f;
		if (bForce || !FMath::IsNearlyEqual(Key, LastAtmosphereKey, 0.002f))
		{
			LastAtmosphereKey = Key;
			Atmosphere->RayleighScatteringScale = BaseRayleigh * S.RayleighScale;
			Atmosphere->MieScatteringScale = BaseMieScattering * S.MieScatteringScale;
			Atmosphere->MieAbsorptionScale = BaseMieAbsorption * S.MieAbsorptionScale;
			Atmosphere->MieAnisotropy = FMath::Clamp(S.MieAnisotropy, 0.f, 0.99f);
			Atmosphere->MieExponentialDistribution = S.MieScaleHeightKm;
			Atmosphere->MarkRenderStateDirty();
		}
	}

	// ---------------------------------------------------------------- fog
	const float Day = UDelhiAtmosphereLibrary::DayFactor(El);
	if (bControlFog)
	{
		Fog->SetVolumetricFog(true);
		Fog->SetFogDensity(S.FogDensity);
		Fog->SetFogHeightFalloff(S.FogHeightFalloff);
		Fog->SetFogInscatteringColor(S.FogInscattering * (FogInscatterScale * (0.03f + 0.97f * Day)));
		Fog->SetVolumetricFogExtinctionScale(S.VolumetricFogExtinctionScale);
		Fog->SetVolumetricFogScatteringDistribution(0.2f);
		const float MaxC = FMath::Max3(S.FogInscattering.R, S.FogInscattering.G, S.FogInscattering.B);
		const FLinearColor Albedo = (S.FogInscattering / FMath::Max(MaxC, 0.01f)) * 0.8f + FLinearColor(0.15f, 0.15f, 0.15f, 0.f);
		Fog->SetVolumetricFogAlbedo(Albedo.ToFColor(true));
		Fog->SetVolumetricFogDistance(S.VisibilityMeters < 1000.f ? 12000.f : 20000.f);
	}

	// ---------------------------------------------------------------- clouds
	if (bControlClouds && !CloudCoverageParameter.IsNone() && Clouds)
	{
		if (!CloudMID && !Clouds->Material.IsNull())
		{
			CloudMID = UMaterialInstanceDynamic::Create(Clouds->Material.LoadSynchronous(), this);
			if (CloudMID) { Clouds->SetMaterial(CloudMID); }
		}
		if (CloudMID) { CloudMID->SetScalarParameterValue(CloudCoverageParameter, S.CloudCover); }
	}

	// ---------------------------------------------------------------- physical exposure
	TargetEV100 = UDelhiAtmosphereLibrary::EstimateExposureEV100(S, El, NightEV100) + ExposureBias;
	if (bControlExposure)
	{
		FPostProcessSettings& P = PostProcess->Settings;
		P.bOverride_AutoExposureMethod = true;
		P.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
		P.bOverride_AutoExposureMinBrightness = true;
		P.AutoExposureMinBrightness = FMath::Clamp(TargetEV100 - ExposureBelow, 1.f, 15.f);
		P.bOverride_AutoExposureMaxBrightness = true;
		P.AutoExposureMaxBrightness = FMath::Clamp(TargetEV100 + ExposureAbove, 1.f, 15.f);
		P.bOverride_AutoExposureSpeedUp = true;
		P.AutoExposureSpeedUp = 3.f;
		P.bOverride_AutoExposureSpeedDown = true;
		P.AutoExposureSpeedDown = 1.f;
		P.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		P.AutoExposureApplyPhysicalCameraExposure = true;
		P.bOverride_AutoExposureBias = true;
		P.AutoExposureBias = 0.f;
		P.bOverride_BloomIntensity = true;
		P.BloomIntensity = 0.4f;
		P.bOverride_VignetteIntensity = true;
		P.VignetteIntensity = 0.25f;
		P.bOverride_FilmGrainIntensity = true;
		P.FilmGrainIntensity = 0.06f;
		P.bOverride_ColorSaturation = true;
		P.ColorSaturation = FVector4(S.Saturation, S.Saturation, S.Saturation, 1.0);
	}
}
