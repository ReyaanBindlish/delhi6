#include "DelhiWeatherSubsystem.h"

#include "DelhiPopulationTypes.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "StoryCalendarSubsystem.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	const TCHAR* const kMonthNames[12] = { TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"), TEXT("May"), TEXT("Jun"),
	                                       TEXT("Jul"), TEXT("Aug"), TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec") };
}

// ------------------------------------------------------------------------------------------------ setup
void UDelhiWeatherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Stream.Initialize(RandomSeed != 0 ? RandomSeed : (int32)(FDateTime::UtcNow().GetTicks() & 0x7fffffff));
	State = UDelhiAtmosphereLibrary::GetBaseState(Target, City);
	FromState = State;
	ToState = State;

	// Soft, quiet lookups: the project works without the imported tables / collection.
	if (!ClimateTable)
	{
		const TCHAR* const Paths[] = { TEXT("/Game/Delhi/Data/DT_ClimateMonth.DT_ClimateMonth"), TEXT("/Game/Data/DT_ClimateMonth.DT_ClimateMonth") };
		for (const TCHAR* Path : Paths)
		{
			if (UDataTable* T = Cast<UDataTable>(FSoftObjectPath(Path).TryLoad())) { ClimateTable = T; break; }
		}
	}
	if (!WeatherCollection)
	{
		WeatherCollection = Cast<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Delhi/Materials/MPC_Weather.MPC_Weather")).TryLoad());
	}
	bCollectionSearched = true;
}

UStoryCalendarSubsystem* UDelhiWeatherSubsystem::GetCalendar() const
{
	const UWorld* W = GetWorld();
	if (W && W->GetGameInstance()) { return W->GetGameInstance()->GetSubsystem<UStoryCalendarSubsystem>(); }
	return nullptr;
}

const UDataTable* UDelhiWeatherSubsystem::GetClimateTable() const
{
	return City == EAtmosphereCity::Karachi ? KarachiClimateTable.Get() : ClimateTable.Get();
}

FDateTime UDelhiWeatherSubsystem::GetLocalDateTime() const
{
	if (const UStoryCalendarSubsystem* Cal = GetCalendar()) { return Cal->GetDate(); }
	return FallbackDate;
}

void UDelhiWeatherSubsystem::SetLocalDateTime(const FDateTime& NewDate)
{
	if (UStoryCalendarSubsystem* Cal = GetCalendar()) { Cal->SetDate(NewDate); }
	else { FallbackDate = NewDate; }
}

// ------------------------------------------------------------------------------------------------ clock
float UDelhiWeatherSubsystem::ClockRate(float Hour) const
{
	// Smooth through dawn (05:00-07:30) and dusk (17:00-19:00) so the golden smog sunset is not skipped.
	const float Day = FMath::SmoothStep(5.f, 7.5f, Hour) * (1.f - FMath::SmoothStep(17.f, 19.f, Hour));
	return FMath::Lerp(NightRate, DayRate, Day);
}

void UDelhiWeatherSubsystem::StepClock(float DeltaTime)
{
	if (bPaused || !bDriveStoryCalendar) { return; }
	const FDateTime Now = GetLocalDateTime();
	const double Hours = Now.GetTimeOfDay().GetTotalHours();
	const double dH = (double)DeltaTime * ClockRate((float)Hours) * TimeScale / 3600.0;

	UStoryCalendarSubsystem* Cal = GetCalendar();
	if (bClockRollsDate)
	{
		if (Cal) { Cal->AdvanceHours((float)dH); }
		else { FallbackDate += FTimespan::FromHours(dH); }
		return;
	}
	// Date moves by story beats only: wrap inside the current day.
	const double NewHours = FMath::Fmod(Hours + dH, 24.0);
	SetLocalDateTime(FDateTime(Now.GetYear(), Now.GetMonth(), Now.GetDay()) + FTimespan::FromHours(NewHours));
}

void UDelhiWeatherSubsystem::SetTimeOfDay(float Hours)
{
	const FDateTime Now = GetLocalDateTime();
	SetLocalDateTime(FDateTime(Now.GetYear(), Now.GetMonth(), Now.GetDay()) + FTimespan::FromHours(FMath::Fmod(FMath::Max(0.f, Hours), 24.f)));
}

void UDelhiWeatherSubsystem::SkipHours(float Hours)
{
	if (Hours <= 0.f) { return; }
	if (UStoryCalendarSubsystem* Cal = GetCalendar()) { Cal->AdvanceHours(Hours); }
	else { FallbackDate += FTimespan::FromHours(Hours); }
}

// ------------------------------------------------------------------------------------------------ weather
void UDelhiWeatherSubsystem::BeginBlend(EDelhiWeather NewWeather, float BlendSeconds)
{
	const EDelhiWeather Previous = Target;
	FromState = State;
	ToState = UDelhiAtmosphereLibrary::GetBaseState(NewWeather, City);
	Current = Target;
	Target = NewWeather;
	if (BlendSeconds <= 0.05f)
	{
		Blend = 1.f; BlendRate = 1000.f; State = ToState;
	}
	else
	{
		Blend = 0.f; BlendRate = 1.f / BlendSeconds;
	}
	if (Previous != NewWeather) { OnWeatherChanged.Broadcast(NewWeather, Previous); }
}

void UDelhiWeatherSubsystem::SetWeather(EDelhiWeather NewWeather, float BlendSeconds)
{
	bManualOverride = true;
	BeginBlend(NewWeather, BlendSeconds);
}

void UDelhiWeatherSubsystem::ReleaseWeatherOverride()
{
	bManualOverride = false;
	ResolveAndApplyWeather(GetLocalDateTime(), AutoBlendSeconds);
}

void UDelhiWeatherSubsystem::SetCity(EAtmosphereCity NewCity)
{
	if (NewCity == City) { return; }
	City = NewCity;
	FromState = State;
	ToState = UDelhiAtmosphereLibrary::GetBaseState(Target, City);
	Blend = 0.f; BlendRate = 1.f / FMath::Max(1.f, AutoBlendSeconds);
	if (bHasRolled && !bManualOverride) { RollDay(GetLocalDateTime(), false); }
}

void UDelhiWeatherSubsystem::SetRandomSeed(int32 NewSeed)
{
	RandomSeed = NewSeed;
	Stream.Initialize(NewSeed != 0 ? NewSeed : (int32)(FDateTime::UtcNow().GetTicks() & 0x7fffffff));
}

void UDelhiWeatherSubsystem::RerollDailyWeather()
{
	RollDay(GetLocalDateTime(), false);
}

void UDelhiWeatherSubsystem::RollDay(const FDateTime& Date, bool bAllowPersistence)
{
	const int32 Month = Date.GetMonth();
	FDelhiClimateMonthRow Row = UDelhiAtmosphereLibrary::GetBuiltinClimate(City, Month);
	if (const UDataTable* T = GetClimateTable())
	{
		if (const FDelhiClimateMonthRow* R = T->FindRow<FDelhiClimateMonthRow>(FName(kMonthNames[Month - 1]), TEXT("ClimateMonth"), false))
		{
			Row = *R;
		}
	}
	const float W[7] = { Row.Clear, Row.Haze, Row.Overcast, Row.Rain, Row.Storm, Row.Fog, Row.Smog };

	// Markov: stay in yesterday's state with probability rho (if it is possible this month), else draw from the month.
	EDelhiDailyState Next = DailyState;
	const bool bCanStay = bHasRolled && bAllowPersistence && W[(int32)DailyState] > 0.f && Stream.FRand() < Row.Persistence;
	if (!bCanStay)
	{
		float Sum = 0.f;
		for (float V : W) { Sum += FMath::Max(0.f, V); }
		float Pick = Stream.FRand() * FMath::Max(Sum, KINDA_SMALL_NUMBER);
		Next = EDelhiDailyState::Clear;
		for (int32 i = 0; i < 7; ++i)
		{
			Pick -= FMath::Max(0.f, W[i]);
			if (Pick <= 0.f) { Next = (EDelhiDailyState)i; break; }
		}
	}
	DailyState = Next;

	// Intra-day structure.
	bHasEpisode = false;
	FogBurnOffHour = Stream.FRandRange(9.5f, 11.5f);
	if (DailyState == EDelhiDailyState::Rain || DailyState == EDelhiDailyState::Storm)
	{
		const bool bRain = DailyState == EDelhiDailyState::Rain;
		const FVector2D Range = bRain ? RainEpisodeHours : StormEpisodeHours;
		const float Dur = Stream.FRandRange((float)Range.X, FMath::Max((float)Range.X, (float)Range.Y));
		const float Lo = bRain ? 6.f : 13.f;
		const float Hi = FMath::Max(Lo, (bRain ? 22.f : 20.f) - Dur);
		EpisodeStartHour = Stream.FRandRange(Lo, Hi);
		EpisodeEndHour = EpisodeStartHour + Dur;
		bHasEpisode = true;
	}

	LastRolledDate = Date;
	const bool bFirst = !bHasRolled;
	bHasRolled = true;
	OnDailyWeatherRolled.Broadcast(DailyState);
	if (!bManualOverride) { ResolveAndApplyWeather(Date, bFirst ? 0.f : AutoBlendSeconds); }
}

void UDelhiWeatherSubsystem::ResolveAndApplyWeather(const FDateTime& Date, float BlendSeconds)
{
	if (!bHasRolled) { return; }
	const float Hour = (float)Date.GetTimeOfDay().GetTotalHours();
	const bool bIn = bHasEpisode && Hour >= EpisodeStartHour && Hour < EpisodeEndHour;
	const EDelhiWeather W = UDelhiAtmosphereLibrary::ResolveDailyWeather(DailyState, City, Date.GetMonth(), bIn, Hour, FogBurnOffHour);
	if (W != Target) { BeginBlend(W, BlendSeconds); }
}

// ------------------------------------------------------------------------------------------------ tick
void UDelhiWeatherSubsystem::Tick(float DeltaTime)
{
	StepClock(DeltaTime);
	const FDateTime Date = GetLocalDateTime();
	TimeOfDayHours = (float)Date.GetTimeOfDay().GetTotalHours();

	if (!bHasRolled || Date.GetDate() != LastRolledDate.GetDate())
	{
		RollDay(Date, true);
	}
	else if (bAutoWeather && !bManualOverride)
	{
		ResolveAndApplyWeather(Date, AutoBlendSeconds);
	}

	Blend = FMath::Min(1.f, Blend + DeltaTime * BlendRate);
	State = FDelhiAtmosphereState::Lerp(FromState, ToState, FMath::SmoothStep(0.f, 1.f, Blend));

	// Surfaces wet quickly and dry slowly.
	const float WantWet = State.Wetness;
	Wetness = FMath::FInterpTo(Wetness, WantWet, DeltaTime, WantWet > Wetness ? 0.2f : 0.01f);

	// Wind: prevailing direction by city/month with gusts. Vector is where the air moves TO, in world space.
	NoiseTime += DeltaTime;
	const float TargetFrom = UDelhiAtmosphereLibrary::PrevailingWindFromBearing(City, Date.GetMonth(), Target);
	WindBearingFrom = FRotator::NormalizeAxis(WindBearingFrom + FMath::FindDeltaAngleDegrees(WindBearingFrom, TargetFrom) * FMath::Clamp(DeltaTime * 0.2f, 0.f, 1.f));
	const float Gust = 0.35f * FMath::PerlinNoise1D(NoiseTime * 0.35f) + 0.15f * FMath::PerlinNoise1D(NoiseTime * 1.7f + 10.f);
	const float Strength = FMath::Max(0.f, State.WindStrength * (1.f + Gust));
	Wind = FVector(Strength, 0.f, 0.f).RotateAngleAxis(NorthYawDegrees + WindBearingFrom + 180.f, FVector::UpVector);

	CollectionTimer += DeltaTime;
	if (CollectionTimer >= 0.05f)
	{
		CollectionTimer = 0.f;
		WriteCollection();
	}
}

void UDelhiWeatherSubsystem::WriteCollection()
{
	UWorld* World = GetWorld();
	if (!World) { return; }
	if (!WeatherCollection)
	{
		if (bCollectionSearched) { return; }
		WeatherCollection = Cast<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Delhi/Materials/MPC_Weather.MPC_Weather")).TryLoad());
		bCollectionSearched = true;
		if (!WeatherCollection) { return; }
	}
	UMaterialParameterCollectionInstance* I = World->GetParameterCollectionInstance(WeatherCollection);
	if (!I) { return; }
	// Missing parameters are skipped silently (older MPC_Weather assets); setup_lighting.py adds the new ones.
	I->SetScalarParameterValue(TEXT("Wetness"), Wetness);
	I->SetScalarParameterValue(TEXT("Dust"), State.Dust);
	I->SetScalarParameterValue(TEXT("Smog"), State.Smog);
	I->SetScalarParameterValue(TEXT("WindX"), Wind.X);
	I->SetScalarParameterValue(TEXT("WindY"), Wind.Y);
	I->SetScalarParameterValue(TEXT("WindStrength"), Wind.Size());
	I->SetScalarParameterValue(TEXT("RainIntensity"), State.RainIntensity);
	I->SetScalarParameterValue(TEXT("Visibility"), State.VisibilityMeters);
	I->SetScalarParameterValue(TEXT("SunElevation"), SunElevation);
	I->SetScalarParameterValue(TEXT("NightFactor"), NightFactor);
	I->SetScalarParameterValue(TEXT("TimeOfDay"), TimeOfDayHours);
}

void UDelhiWeatherSubsystem::ReportSun(float ElevationDeg, float AzimuthDeg)
{
	SunElevation = ElevationDeg;
	SunAzimuth = AzimuthDeg;
	NightFactor = 1.f - UDelhiAtmosphereLibrary::DayFactor(ElevationDeg);
}

FVector UDelhiWeatherSubsystem::GetWindAtLocation(const FVector& WorldLocation) const
{
	const float N = FMath::PerlinNoise2D(FVector2D(WorldLocation.X, WorldLocation.Y) * 0.0005 + FVector2D(NoiseTime * 0.1f, 0.f));
	return Wind * (1.f + 0.25f * N);
}

float UDelhiWeatherSubsystem::GetTractionMultiplier() const
{
	return FMath::Lerp(1.f, 0.55f, FMath::Clamp(Wetness, 0.f, 1.f)) * (1.f - 0.1f * FMath::Clamp(State.Dust, 0.f, 1.f));
}
