#include "World/DelhiWeatherController.h"

#include "DelhiWeatherSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "StoryCalendarSubsystem.h"

ADelhiWeatherController::ADelhiWeatherController()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

UDelhiWeatherSubsystem* ADelhiWeatherController::GetWeatherSubsystem() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetSubsystem<UDelhiWeatherSubsystem>() : nullptr;
}

void ADelhiWeatherController::BeginPlay()
{
	Super::BeginPlay();
	UDelhiWeatherSubsystem* Sub = GetWeatherSubsystem();
	if (!Sub) { return; }

	Sub->SetCity(City);
	Sub->bAutoWeather = bAutoWeather;
	Sub->SetRandomSeed(RandomSeed);
	Sub->DayRate = DayRate;
	Sub->NightRate = NightRate;
	Sub->TimeScale = TimeScale;
	Sub->bPaused = bPaused;
	if (WeatherCollection) { Sub->WeatherCollection = WeatherCollection; }

	if (bApplyStartTime)
	{
		const int32 Y = FMath::Clamp(StartYear, 1901, 2200);
		const int32 M = FMath::Clamp(StartMonth, 1, 12);
		const int32 D = FMath::Clamp(StartDay, 1, FDateTime::DaysInMonth(Y, M));
		const FDateTime Start = FDateTime(Y, M, D) + FTimespan::FromHours(FMath::Clamp(StartHour, 0.f, 23.99f));
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UStoryCalendarSubsystem* Cal = GI->GetSubsystem<UStoryCalendarSubsystem>()) { Cal->SetDate(Start); }
		}
		else
		{
			Sub->FallbackDate = Start;
		}
	}

	Sub->OnWeatherChanged.AddDynamic(this, &ADelhiWeatherController::HandleWeatherChanged);
	Sub->OnDailyWeatherRolled.AddDynamic(this, &ADelhiWeatherController::HandleDailyRolled);

	if (bForceInitialWeather) { Sub->SetWeather(InitialWeather, 0.f); }
}

void ADelhiWeatherController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UDelhiWeatherSubsystem* Sub = GetWeatherSubsystem())
	{
		Sub->OnWeatherChanged.RemoveDynamic(this, &ADelhiWeatherController::HandleWeatherChanged);
		Sub->OnDailyWeatherRolled.RemoveDynamic(this, &ADelhiWeatherController::HandleDailyRolled);
	}
	Super::EndPlay(EndPlayReason);
}

void ADelhiWeatherController::ForceWeather(EDelhiWeather Weather, float BlendSeconds)
{
	if (UDelhiWeatherSubsystem* Sub = GetWeatherSubsystem()) { Sub->SetWeather(Weather, BlendSeconds); }
}

void ADelhiWeatherController::ReleaseOverride()
{
	if (UDelhiWeatherSubsystem* Sub = GetWeatherSubsystem()) { Sub->ReleaseWeatherOverride(); }
}

void ADelhiWeatherController::SetTimeOfDay(float Hours)
{
	if (UDelhiWeatherSubsystem* Sub = GetWeatherSubsystem()) { Sub->SetTimeOfDay(Hours); }
}

float ADelhiWeatherController::GetWetness() const { const UDelhiWeatherSubsystem* S = GetWeatherSubsystem(); return S ? S->GetWetness() : 0.f; }
float ADelhiWeatherController::GetRainIntensity() const { const UDelhiWeatherSubsystem* S = GetWeatherSubsystem(); return S ? S->GetRainIntensity() : 0.f; }
float ADelhiWeatherController::GetTractionMultiplier() const { const UDelhiWeatherSubsystem* S = GetWeatherSubsystem(); return S ? S->GetTractionMultiplier() : 1.f; }
FVector ADelhiWeatherController::GetWind() const { const UDelhiWeatherSubsystem* S = GetWeatherSubsystem(); return S ? S->GetWind() : FVector::ZeroVector; }

void ADelhiWeatherController::HandleWeatherChanged(EDelhiWeather NewWeather, EDelhiWeather PreviousWeather)
{
	OnWeatherStateChanged(NewWeather, PreviousWeather);
}

void ADelhiWeatherController::HandleDailyRolled(EDelhiDailyState DailyState)
{
	OnDailyStateRolled(DailyState);
}
