#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FallTrackerComponent.generated.h"

class ACharacter;
class USkeletalMeshComponent;
class UDayaVitalsComponent;

UENUM(BlueprintType)
enum class EFallSeverity : uint8
{
	None,
	/** Under 3 m. */
	Minor,
	/** 3 to 6 m: injury. */
	Injury,
	/** Above 6 m: likely death. */
	Lethal
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFallCompleted, float, HeightMeters, EFallSeverity, Severity);

/**
 * Measures how far a character fell (capsule or ragdoll) and reports it to DayaVitalsComponent::ApplyFall.
 * Capsule mode: peak feet height while falling, reported on ACharacter::LandedDelegate.
 * Ragdoll mode (mesh simulating): pelvis height; lands when downward speed collapses after a fall.
 */
UCLASS(ClassGroup = (Physics), meta = (BlueprintSpawnableComponent))
class OLDDELHI_API UFallTrackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFallTrackerComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Falls shorter than this are ignored (stairs, hops). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall") float MinReportMeters = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall") float InjuryMeters = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall") float LethalMeters = 6.f;
	/** Bone used in ragdoll mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall") FName RagdollRootBone = TEXT("pelvis");
	/** Forward the result to DayaVitalsComponent on the same actor (if present). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall") bool bReportToVitals = true;

	UPROPERTY(BlueprintAssignable, Category = "Fall") FOnFallCompleted OnFallCompleted;

	/** Force tracking to start from the current height (called by URailingComponent). */
	UFUNCTION(BlueprintCallable, Category = "Fall") void BeginTracking();
	/** Manual landing report (e.g. a vehicle crash, or custom ragdoll code). */
	UFUNCTION(BlueprintCallable, Category = "Fall") void ReportLanding(float HeightMeters);

	UFUNCTION(BlueprintPure, Category = "Fall") EFallSeverity Classify(float HeightMeters) const;
	UFUNCTION(BlueprintPure, Category = "Fall") bool IsTracking() const { return bTracking; }

private:
	UFUNCTION() void OnLanded(const FHitResult& Hit);

	float GetCurrentHeightCm() const;

	TWeakObjectPtr<ACharacter> Character;
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;
	bool bTracking = false;
	bool bRagFalling = false;
	float PeakZ = 0.f;
	float LastZ = 0.f;
};
