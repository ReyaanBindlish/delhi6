#include "DayaVitalsComponent.h"

UDayaVitalsComponent::UDayaVitalsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f; // 20 Hz is plenty for vitals
}

float UDayaVitalsComponent::GetMaxStamina() const
{
	float Max = BaseMaxStamina;
	switch (Disease)
	{
	case EDiseaseStage::Cough:
	case EDiseaseStage::WeightLoss:
	case EDiseaseStage::Untreated:
		Max -= MonthsCough * MaxLossPerMonth * (Disease == EDiseaseStage::Cough ? 1.f : 1.4f);
		break;
	case EDiseaseStage::Diagnosed:
		Max -= MonthsCough * MaxLossPerMonth * 1.4f + WeeksDiagnosed * MaxLossPerWeek * (bOnMedication ? 0.3f : 1.f);
		break;
	default:
		break;
	}
	// Torso injuries also cap stamina.
	for (const FInjury& I : Injuries)
	{
		if (I.Part == EBodyPart::Torso) { Max -= 15.f * I.Severity; }
	}
	return FMath::Max(MinMaxStamina, Max);
}

void UDayaVitalsComponent::SetDiseaseProgress(float MonthsSinceCough, float WeeksSinceDiagnosis)
{
	MonthsCough = FMath::Max(0.f, MonthsSinceCough);
	WeeksDiagnosed = FMath::Max(0.f, WeeksSinceDiagnosis);
}

bool UDayaVitalsComponent::SpendStamina(float Amount)
{
	if (IsInputLocked() || Stamina <= 0.f) { return false; }
	Stamina = FMath::Max(0.f, Stamina - Amount);
	return true;
}

void UDayaVitalsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CoughLockRemaining > 0.f) { CoughLockRemaining -= DeltaTime; }
	if (CoughCooldown > 0.f) { CoughCooldown -= DeltaTime; }

	float Regen = RegenPerSecond;
	if (HoursSinceMeal > 18.f) { Regen *= 0.5f; }                 // hunger past 18 h halves regen
	if (Disease >= EDiseaseStage::WeightLoss) { Regen *= 0.75f; }
	if (IsInputLocked()) { Regen = 0.f; }

	const float Max = GetMaxStamina();
	Stamina = FMath::Clamp(Stamina + (Regen - HeatDrainPerSecond) * DeltaTime, 0.f, Max);

	MaybeCough(DeltaTime);
}

void UDayaVitalsComponent::MaybeCough(float DeltaTime)
{
	if (Disease == EDiseaseStage::Healthy || IsInputLocked() || CoughCooldown > 0.f) { return; }
	const float Frac = Stamina / FMath::Max(1.f, GetMaxStamina());
	if (Frac > CoughThreshold) { return; }
	// Hazard rises as stamina falls and the disease advances.
	const float StageScale = 0.25f * static_cast<float>(Disease);
	const float PerSecond = (1.f - Frac / CoughThreshold) * StageScale;
	if (Rng.FRand() < PerSecond * DeltaTime)
	{
		const float Lock = FMath::Lerp(2.f, 4.f, Rng.FRand());   // bible: 2 to 4 seconds
		CoughLockRemaining = Lock;
		CoughCooldown = Lock + 6.f;
		OnCoughFit.Broadcast(Lock);
	}
}

void UDayaVitalsComponent::ApplyInjury(const FInjury& Injury, float HealthDamage)
{
	Injuries.Add(Injury);
	Health = FMath::Max(0.f, Health - HealthDamage);   // never regenerates
	if (Health <= 0.f) { OnDeath.Broadcast(TEXT("Injuries")); }
}

void UDayaVitalsComponent::ApplyFall(float HeightMeters)
{
	if (HeightMeters <= 3.f) { return; }
	if (HeightMeters > 6.f)
	{
		// Above 6 m: likely death. 12 m (four floors) is certain.
		const float PDeath = FMath::GetMappedRangeValueClamped(FVector2D(6.f, 10.f), FVector2D(0.6f, 1.f), HeightMeters);
		if (Rng.FRand() < PDeath)
		{
			Health = 0.f;
			OnDeath.Broadcast(FString::Printf(TEXT("Fall %.1f m"), HeightMeters));
			return;
		}
	}
	FInjury Leg;
	Leg.Part = Rng.FRand() < 0.5f ? EBodyPart::LeftLeg : EBodyPart::RightLeg;
	Leg.Severity = FMath::GetMappedRangeValueClamped(FVector2D(3.f, 6.f), FVector2D(0.3f, 0.9f), HeightMeters);
	Leg.DaysToHeal = HeightMeters > 4.5f ? -1.f : 10.f;   // bad falls need a doctor
	Leg.Permanent = 0.2f;
	ApplyInjury(Leg, 15.f * HeightMeters);
}

void UDayaVitalsComponent::OnDayPassed(bool bSawDoctor)
{
	for (int32 i = Injuries.Num() - 1; i >= 0; --i)
	{
		FInjury& I = Injuries[i];
		if (I.DaysToHeal < 0.f && bSawDoctor) { I.DaysToHeal = 7.f; }   // doctor makes it healable
		if (I.DaysToHeal < 0.f) { continue; }
		I.DaysToHeal -= 1.f;
		if (I.DaysToHeal <= 0.f)
		{
			if (I.Permanent > 0.f) { I.Severity *= I.Permanent; I.DaysToHeal = -1.f; I.Permanent = 0.f; }
			else { Injuries.RemoveAt(i); }
		}
	}
}

float UDayaVitalsComponent::GetMoveSpeedMultiplier() const
{
	float M = 1.f;
	for (const FInjury& I : Injuries)
	{
		if (I.Part == EBodyPart::LeftLeg || I.Part == EBodyPart::RightLeg) { M *= (1.f - I.Severity); }
	}
	if (HoursSinceMeal > 18.f) { M *= 0.9f; }
	return FMath::Clamp(M, 0.25f, 1.f);
}

void UDayaVitalsComponent::SetHeatExposure(float DegreesC, bool bInShade)
{
	// Delhi in June / Karachi in August: open-air heat drains stamina above ~35 C.
	HeatDrainPerSecond = bInShade ? 0.f : FMath::Max(0.f, DegreesC - 35.f) * 0.6f;
}
