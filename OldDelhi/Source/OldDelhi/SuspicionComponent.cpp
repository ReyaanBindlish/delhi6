#include "SuspicionComponent.h"

USuspicionComponent::USuspicionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CauseWeights.Add(ESuspicionCause::AccentSlip, 8.f);
	CauseWeights.Add(ESuspicionCause::LegendHesitation, 12.f);
	CauseWeights.Add(ESuspicionCause::WrongLegendAnswer, 35.f);
	CauseWeights.Add(ESuspicionCause::WeaponCarriedEarly, 20.f);
	CauseWeights.Add(ESuspicionCause::ThumbTell, 15.f);
	CauseWeights.Add(ESuspicionCause::SeenAtWrongPlace, 10.f);
	CauseWeights.Add(ESuspicionCause::AskedTooMuch, 6.f);
}

void USuspicionComponent::AddSuspicion(ESuspicionCause Cause, float Scale)
{
	const float* W = CauseWeights.Find(Cause);
	Suspicion = FMath::Clamp(Suspicion + (W ? *W : 5.f) * Scale, 0.f, 100.f);
	Counts.FindOrAdd(Cause) += 1;

	if (!bWatching && Suspicion >= WatchThreshold)
	{
		bWatching = true;
		OnStartWatching.Broadcast(GetOwner(), Suspicion);
	}
	if (!bExposed && Suspicion >= ExposeThreshold)
	{
		bExposed = true;
		OnExposed.Broadcast(GetOwner(), Suspicion);
	}
}

void USuspicionComponent::DailyDecay()
{
	if (bExposed) { return; } // once exposed, nothing is forgotten
	Suspicion *= (1.f - FMath::Clamp(DecayPerDay, 0.f, 1.f));
	if (bWatching && Suspicion < WatchThreshold * 0.5f) { bWatching = false; }
}

int32 USuspicionComponent::CountOf(ESuspicionCause Cause) const
{
	const int32* C = Counts.Find(Cause);
	return C ? *C : 0;
}
