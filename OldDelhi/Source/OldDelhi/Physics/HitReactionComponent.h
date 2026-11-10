#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "HitReactionComponent.generated.h"

class UPhysicalAnimationComponent;
class USkeletalMeshComponent;
class UCapsuleComponent;
class UHitReactionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRagdollStateChanged);

/**
 * Hit reactions for a character: partial physical animation for small hits (bone chain goes loose, blends back),
 * full ragdoll for big ones. Needs a Physics Asset on the skeletal mesh. Ragdolls are counted by
 * UDelhiRagdollBudgetSubsystem (cap 8: the oldest is put to sleep, research/03 section 1.2).
 */
UCLASS(ClassGroup = (Physics), meta = (BlueprintSpawnableComponent))
class OLDDELHI_API UHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHitReactionComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Real body mass in kg. The physics asset is rescaled to this if it is more than 25% off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction") float BodyMassKg = 70.f;
	/** Impulse (N*s) above which a hit becomes a full ragdoll. A hard shove is ~80-150, a bullet only ~3. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction") float FullRagdollThresholdNs = 180.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction") float PartialBlendOutSeconds = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction") float PartialPeakBlendWeight = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction") FName RagdollCollisionProfile = TEXT("Ragdoll");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReaction") FName RagdollRootBone = TEXT("pelvis");

	/**
	 * Apply a hit. ImpulseNs is the real-world impulse in newton-seconds (converted to UE units internally).
	 * Small impulses use physical animation below Bone; large ones (or a dead character) go full ragdoll.
	 */
	UFUNCTION(BlueprintCallable, Category = "HitReaction")
	void ApplyHit(FName Bone, FVector HitLocation, FVector ImpulseDirection, float ImpulseNs);

	UFUNCTION(BlueprintCallable, Category = "HitReaction")
	void EnterFullRagdoll(FVector LinearVelocityCmS);

	/** Stand back up from a ragdoll (not for the dead). Snaps the capsule to the pelvis. */
	UFUNCTION(BlueprintCallable, Category = "HitReaction")
	void RecoverFromRagdoll();

	/** Stop simulating but keep the pose (used by the budget when over the cap). */
	void FreezeRagdoll();

	UFUNCTION(BlueprintPure, Category = "HitReaction") bool IsRagdoll() const { return bRagdoll; }
	UFUNCTION(BlueprintPure, Category = "HitReaction") bool IsFrozen() const { return bFrozen; }

	UPROPERTY(BlueprintAssignable, Category = "HitReaction") FOnRagdollStateChanged OnRagdollStarted;
	UPROPERTY(BlueprintAssignable, Category = "HitReaction") FOnRagdollStateChanged OnRagdollEnded;

private:
	struct FActiveBlend
	{
		FName Bone;
		float TimeLeft = 0.f;
		float Total = 1.f;
	};

	void NormalizeMass();

	UPROPERTY(Transient) TObjectPtr<UPhysicalAnimationComponent> PhysAnim;
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;
	TWeakObjectPtr<UCapsuleComponent> Capsule;
	TArray<FActiveBlend> Blends;
	FTransform SavedMeshRelative;
	FName SavedCollisionProfile;
	bool bRagdoll = false;
	bool bFrozen = false;
	bool bMassNormalized = false;
};

/** Counts active ragdolls and sleeps the oldest when over the cap (8, research/03). */
UCLASS()
class OLDDELHI_API UDelhiRagdollBudgetSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Budget") int32 MaxActiveRagdolls = 8;

	void Register(UHitReactionComponent* Comp);
	void Unregister(UHitReactionComponent* Comp);

	UFUNCTION(BlueprintPure, Category = "Physics|Budget") int32 GetActiveRagdollCount() const;

private:
	TArray<TWeakObjectPtr<UHitReactionComponent>> Active; // oldest first
};
