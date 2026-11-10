#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DhCrowdTypes.h"
#include "DhCrowdCharacter.generated.h"

class UAnimSequence;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkeletalMesh;

/** Loaded stand-in assets for one gender. Kept alive by UDhCrowdSubsystem. */
USTRUCT()
struct FDhCrowdLoadedVisual
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<USkeletalMesh> Mesh = nullptr;
	UPROPERTY() TObjectPtr<UAnimSequence> Walk = nullptr;
	UPROPERTY() TObjectPtr<UAnimSequence> Idle = nullptr;
	UPROPERTY() bool bFemale = false;
};

/**
 * Pooled near-tier crowd character (the 12 hero + ~40 skeletal tiers). It follows a virtual agent that the
 * crowd subsystem advances along the lane network, so the same agent can swap between an abstract point, an
 * instanced mesh and this actor without a visible jump. Uses a single-node animation (walk / idle) on the UE
 * Manny/Quinn mannequins until MetaHuman crowd characters replace them (see docs/CROWD.md).
 */
UCLASS(NotBlueprintable, Transient)
class OLDDELHI_API ADhCrowdCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADhCrowdCharacter();

	/** Called when the actor leaves the pool for an agent. */
	void Acquire(const FDhCrowdAgent& Agent, const FDhCrowdLoadedVisual& Visual, UMaterialInterface* MaterialOverride,
		FName TintParam, FName SkinParam, bool bHero);
	/** Returns to the pool: hidden, collision and ticks off. */
	void Release();
	bool IsInUse() const { return bInUse; }

	/** Follow the virtual agent. TargetPos is the lane pose in world cm. */
	void Drive(const FVector& TargetPos, const FVector& TravelDir, float SpeedCms, EDhCrowdActivity Activity, float Dt);

	/** Re-tier without respawning (hero gets full-rate animation and avoidance). */
	void SetHero(bool bHero);

	/** Feet height under the capsule (for writing back to the agent). */
	float GetFeetZ() const;

private:
	void PlayLoop(UAnimSequence* Anim, float PlayRate);

	UPROPERTY(Transient) TObjectPtr<UAnimSequence> WalkAnim = nullptr;
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> IdleAnim = nullptr;
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> CurrentAnim = nullptr;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Mids;

	bool bInUse = false;
	bool bHeroNow = false;
	float TeleportDistanceCm = 350.f;
};
