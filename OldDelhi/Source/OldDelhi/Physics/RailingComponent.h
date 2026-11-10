#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "RailingComponent.generated.h"

class ACharacter;

UENUM(BlueprintType)
enum class ERailStandard : uint8
{
	/** Cheap builder-flat rail, 90 cm (the Act 6 balcony). */
	Builder90,
	/** Building-code rail, 110 cm. */
	Code110,
	Custom
};

USTRUCT(BlueprintType)
struct OLDDELHI_API FRailEvaluation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rail") bool bNearRail = false;
	UPROPERTY(BlueprintReadOnly, Category = "Rail") bool bGoesOver = false;
	/** Character centre of mass above the floor the rail stands on (m). */
	UPROPERTY(BlueprintReadOnly, Category = "Rail") float ComHeightM = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Rail") float RailHeightM = 0.f;
	/** COM height minus rail height; > 0 means the COM is already above the pivot. */
	UPROPERTY(BlueprintReadOnly, Category = "Rail") float LeverM = 0.f;
	/** Horizontal COM distance behind the rail pivot (m). */
	UPROPERTY(BlueprintReadOnly, Category = "Rail") float ComDistanceM = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Rail") float RequiredSpeedMS = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Rail") float EffectivePushSpeedMS = 0.f;
};

/**
 * A guard rail that can be fallen over. The character is treated as an inverted pendulum pivoting on the rail
 * top: to go over, the push must lift the centre of mass (COM) through the point directly above the pivot.
 *
 *   lever = COMheight - railHeight;  r = sqrt(d^2 + lever^2)   (d = horizontal COM-to-pivot distance)
 *   energy needed per kg = g * (r - lever)
 *   goes over  <=>  0.5 * eff * v_push^2  >=  g * (r - lever) * gripFactor
 *
 * With COM ~0.98 m (172 cm man): a 90 cm rail needs ~1.6 m/s (a hard shove while he is staggering),
 * a 110 cm rail needs ~3 m/s (not reachable by a shove). While DayaVitalsComponent::IsInputLocked() (the cough
 * lock) he cannot grab the rail, so the grip factor is 1; otherwise GripRecoveryFactor (> 1) applies.
 *
 * Placement: the component ORIGIN is the middle of the rail (half rail height above the floor); the box extent
 * is rebuilt from RailHeight/Length/Thickness. Rail runs along local X; the drop side is local +Y
 * (flip with bDropSideIsNegativeY).
 */
UCLASS(ClassGroup = (Physics), meta = (BlueprintSpawnableComponent))
class OLDDELHI_API URailingComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	URailingComponent();
	virtual void OnRegister() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail") ERailStandard Standard = ERailStandard::Builder90;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail", meta = (EditCondition = "Standard == ERailStandard::Custom")) float CustomRailHeightCm = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail") float RailLengthCm = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail") float RailThicknessCm = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail") bool bDropSideIsNegativeY = false;

	/** Fraction of standing height at which the COM sits (anthropometric ~0.57). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail|Tuning") float ComHeightFraction = 0.57f;
	/** COM horizontal offset in front of the capsule axis when the body touches the rail (half body depth, cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail|Tuning") float BodyHalfDepthCm = 15.f;
	/** Fraction of push kinetic energy that turns into rotation over the rail (rest lost to the legs/feet). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail|Tuning") float PushEfficiency = 0.7f;
	/** Extra required energy when the character can still grab the rail (not cough-locked). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail|Tuning") float GripRecoveryFactor = 1.5f;
	/** Seconds the rail ignores the victim after a successful push over (so the capsule can pass). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail|Tuning") float PassThroughSeconds = 2.5f;
	/** If true the victim goes full ragdoll (UHitReactionComponent) once over. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail|Tuning") bool bRagdollOnFall = true;

	UFUNCTION(BlueprintPure, Category = "Rail") float GetRailHeightCm() const;
	UFUNCTION(BlueprintPure, Category = "Rail") FVector GetDropDirection() const;

	/** Pure maths so Blueprints, tests and tools can call it. All values in metres / m/s. */
	UFUNCTION(BlueprintPure, Category = "Rail")
	static bool EvaluateOverRail(float ComHeightM, float RailHeightM, float ComDistanceM, float PushSpeedMS,
	                             float InPushEfficiency, float GripFactor, FRailEvaluation& OutEval);

	/** Evaluate without acting. PushVelocityCmS is the velocity the shove would give (world cm/s). */
	UFUNCTION(BlueprintCallable, Category = "Rail")
	FRailEvaluation EvaluateCharacter(ACharacter* Victim, FVector PushVelocityCmS) const;

	/**
	 * Apply a shove. If the COM goes over the rail the victim is launched through it, fall tracking starts
	 * (UFallTrackerComponent, added automatically) and the landing feeds DayaVitalsComponent::ApplyFall.
	 * Returns true if the victim went over.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rail")
	bool TryShove(ACharacter* Victim, FVector ShoveVelocityCmS, FRailEvaluation& OutEval);
};
