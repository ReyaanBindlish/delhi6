#include "DelhiWeatherSubsystem.h"

#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"

namespace
{
	struct FWeatherTargets
	{
		float Wetness;
		float Dust;
		float Smog;
		float WindStrength;
	};

	FWeatherTargets TargetsFor(EDelhiWeather W)
	{
		switch (W)
		{
		case EDelhiWeather::Clear:     return { 0.0f, 0.10f, 0.15f, 2.f };
		case EDelhiWeather::Haze:      return { 0.0f, 0.25f, 0.45f, 2.f };
		case EDelhiWeather::Smog:      return { 0.0f, 0.30f, 0.90f, 1.f };
		case EDelhiWeather::Fog:       return { 0.2f, 0.10f, 1.00f, 0.5f };
		case EDelhiWeather::Rain:      return { 0.8f, 0.00f, 0.20f, 5.f };
		case EDelhiWeather::Monsoon:   return { 1.0f, 0.00f, 0.15f, 8.f };
		case EDelhiWeather::DustStorm: return { 0.0f, 1.00f, 0.60f, 25.f };
		}
		return { 0.f, 0.f, 0.f, 0.f };
	}
}

void UDelhiWeatherSubsystem::SetWeather(EDelhiWeather NewWeather, float BlendSeconds)
{
	Current = Target;
	Target = NewWeather;
	Blend = 0.f;
	BlendRate = BlendSeconds > KINDA_SMALL_NUMBER ? 1.f / BlendSeconds : 1000.f;
}

void UDelhiWeatherSubsystem::Tick(float DeltaTime)
{
	// Clock: faster at night so nights pass quickly.
	const bool bNight = TimeOfDayHours < 6.f || TimeOfDayHours > 19.f;
	TimeOfDayHours = FMath::Fmod(TimeOfDayHours + DeltaTime * (bNight ? NightRate : DayRate) / 60.f, 24.f);

	Blend = FMath::Min(1.f, Blend + DeltaTime * BlendRate);
	const FWeatherTargets A = TargetsFor(Current);
	const FWeatherTargets B = TargetsFor(Target);
	const float T = FMath::SmoothStep(0.f, 1.f, Blend);

	// Wetness dries slower than it wets.
	const float WantWet = FMath::Lerp(A.Wetness, B.Wetness, T);
	const float WetSpeed = WantWet > Wetness ? 0.2f : 0.01f;
	Wetness = FMath::FInterpTo(Wetness, WantWet, DeltaTime, WetSpeed);
	Dust = FMath::Lerp(A.Dust, B.Dust, T);
	Smog = FMath::Lerp(A.Smog, B.Smog, T);
	const float Strength = FMath::Lerp(A.WindStrength, B.WindStrength, T);
	Wind = FVector(Strength, 0.f, 0.f).RotateAngleAxis(225.f, FVector::UpVector); // prevailing NW wind (blowing towards SE)

	if (WeatherCollection)
	{
		UWorld* World = GetWorld();
		UKismetMaterialLibrary::SetScalarParameterValue(World, WeatherCollection, TEXT("Wetness"), Wetness);
		UKismetMaterialLibrary::SetScalarParameterValue(World, WeatherCollection, TEXT("Dust"), Dust);
		UKismetMaterialLibrary::SetScalarParameterValue(World, WeatherCollection, TEXT("Smog"), Smog);
		UKismetMaterialLibrary::SetScalarParameterValue(World, WeatherCollection, TEXT("WindX"), Wind.X);
		UKismetMaterialLibrary::SetScalarParameterValue(World, WeatherCollection, TEXT("WindY"), Wind.Y);
		UKismetMaterialLibrary::SetScalarParameterValue(World, WeatherCollection, TEXT("WindStrength"), Strength);
	}
}
