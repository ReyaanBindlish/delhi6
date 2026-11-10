#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SuspicionComponent.generated.h"

UENUM(BlueprintType)
enum class ESuspicionCause : uint8
{
	AccentSlip,
	LegendHesitation,   // e.g. Qureshi's eight-second Kasur station question
	WrongLegendAnswer,
	WeaponCarriedEarly, // "never carry a weapon in the first six months"
	ThumbTell,          // cracking the left thumb when lying
	SeenAtWrongPlace,
	AskedTooMuch
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSuspicionThreshold, AActor*, Watcher, float, Value);

/**
 * Hidden per-NPC suspicion of the player (bible: "It never shows on the HUD").
 * Lives on each Karachi NPC that matters; Qureshi gets a long memory (low decay, thumb tell weighted).
 */
UCLASS(ClassGroup = (Story), meta = (BlueprintSpawnableComponent))
class OLDDELHI_API USuspicionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USuspicionComponent();

	UFUNCTION(BlueprintCallable, Category = "Story|Suspicion") void AddSuspicion(ESuspicionCause Cause, float Scale = 1.f);
	/** Call once per in-game day. */
	UFUNCTION(BlueprintCallable, Category = "Story|Suspicion") void DailyDecay();
	UFUNCTION(BlueprintPure, Category = "Story|Suspicion") float GetSuspicion() const { return Suspicion; }
	UFUNCTION(BlueprintPure, Category = "Story|Suspicion") int32 CountOf(ESuspicionCause Cause) const;

	/** Points per cause (tunable per NPC). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Suspicion") TMap<ESuspicionCause, float> CauseWeights;
	/** Fraction lost per in-game day (Qureshi ~0.01, a tea-stall owner ~0.15). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Suspicion") float DecayPerDay = 0.08f;
	/** At these values the NPC starts testing (Watch) and then acts (Expose). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Suspicion") float WatchThreshold = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Suspicion") float ExposeThreshold = 85.f;

	UPROPERTY(BlueprintAssignable, Category = "Story|Suspicion") FOnSuspicionThreshold OnStartWatching;
	UPROPERTY(BlueprintAssignable, Category = "Story|Suspicion") FOnSuspicionThreshold OnExposed;

private:
	UPROPERTY(SaveGame) float Suspicion = 0.f;
	UPROPERTY(SaveGame) TMap<ESuspicionCause, int32> Counts;
	bool bWatching = false;
	bool bExposed = false;
};
