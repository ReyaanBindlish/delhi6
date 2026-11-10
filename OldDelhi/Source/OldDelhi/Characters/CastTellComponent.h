#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SuspicionComponent.h"
#include "CastTellComponent.generated.h"

class UAnimMontage;

/**
 * A character's involuntary tell. Daya cracks his left thumb when he lies: an additive left-hand montage
 * that every observer can see, and that feeds USuspicionComponent::AddSuspicion(ThumbTell) on observers who
 * are close enough and facing him. The player never sees a prompt; the cost shows up later (bible: Karachi
 * suspicion is hidden and "feels like it came from nowhere").
 */
USTRUCT(BlueprintType)
struct OLDDELHI_API FCastTell
{
	GENERATED_BODY()

	/** Left-hand additive montage (slot "UpperBody" or "LeftHand"). Optional: no montage still raises suspicion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UAnimMontage> Montage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ESuspicionCause Cause = ESuspicionCause::ThumbTell;
	/** Multiplier passed to AddSuspicion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SuspicionScale = 1.f;
	/** Minimum seconds between two plays (a lie told twice in a breath is one tell). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CooldownSeconds = 4.f;
	/** Observers farther than this cannot notice it (the thumb is small). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxObserveDistanceCm = 350.f;
	/** Half-angle of the observer's view cone towards the owner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ObserveHalfAngleDegrees = 70.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastTell, FName, TellId, int32, ObserversWhoSawIt);

UCLASS(ClassGroup = (Story), meta = (BlueprintSpawnableComponent))
class OLDDELHI_API UCastTellComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCastTellComponent();

	/**
	 * Play the tell and add suspicion on every observer that can see it. Call from dialogue when the player
	 * lies (not from a menu). Returns how many observers noticed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cast|Tell") int32 TriggerTell(FName TellId, const TArray<AActor*>& Observers);

	/** Convenience for Daya: TriggerTell("ThumbCrack", Observers). */
	UFUNCTION(BlueprintCallable, Category = "Cast|Tell") int32 CrackThumb(const TArray<AActor*>& Observers);

	UFUNCTION(BlueprintPure, Category = "Cast|Tell") bool IsOnCooldown(FName TellId) const;

	UPROPERTY(BlueprintAssignable, Category = "Cast|Tell") FOnCastTell OnTell;

	/** Tell id -> definition. Default contains "ThumbCrack". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast|Tell") TMap<FName, FCastTell> Tells;

private:
	/** World time of the last play per tell id. */
	TMap<FName, double> LastPlayed;
};
