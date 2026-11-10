#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DayaVitalsComponent.generated.h"

UENUM(BlueprintType)
enum class EBodyPart : uint8 { Head, Torso, LeftArm, RightArm, LeftLeg, RightLeg };

UENUM(BlueprintType)
enum class EDiseaseStage : uint8
{
	Healthy,        // Acts 1-3, Act 4 Phase 1
	Cough,          // Act 4 Phase 2: max stamina shrinks monthly
	WeightLoss,     // Act 4 Phase 3: night sweats, faster drain
	Untreated,      // Act 5
	Diagnosed       // Act 6: hard ceiling drops weekly
};

USTRUCT(BlueprintType)
struct FInjury
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) EBodyPart Part = EBodyPart::Torso;
	/** 0-1. A baton to the knee is ~0.5 (halves walking speed for three in-game days). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Severity = 0.3f;
	/** In-game days left; < 0 = never heals without a doctor (bible: "or not at all"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DaysToHeal = 3.f;
	/** Fraction of severity that never heals (an Act 2 knee is still slower in Act 4). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Permanent = 0.1f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoughFit, float, LockSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVitalsDeath, FString, Cause);

/**
 * Daya's body as a ledger (bible, "Physics and realism rules"):
 *  - Health never regenerates; injuries heal over in-game days, with a doctor, or not at all.
 *  - One stamina meter drives running, fighting, climbing. Its MAX shrinks with the disease stage.
 *  - Coughing fits trigger at low stamina and lock input for 2-4 s (this is what loses the balcony fight).
 *  - Hunger past 18 h halves stamina regeneration; open-air heat drains stamina.
 *  - Falls: above 3 m injury, above 6 m likely death.
 */
UCLASS(ClassGroup = (Story), meta = (BlueprintSpawnableComponent))
class OLDDELHI_API UDayaVitalsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDayaVitalsComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ---- stamina -------------------------------------------------------------------------------
	/** Spend stamina for an action (sprint per second, punch, climb). Returns false if exhausted. */
	UFUNCTION(BlueprintCallable, Category = "Story|Vitals") bool SpendStamina(float Amount);
	UFUNCTION(BlueprintPure, Category = "Story|Vitals") float GetStamina() const { return Stamina; }
	UFUNCTION(BlueprintPure, Category = "Story|Vitals") float GetMaxStamina() const;
	UFUNCTION(BlueprintPure, Category = "Story|Vitals") bool IsInputLocked() const { return CoughLockRemaining > 0.f; }

	// ---- disease (driven by the story calendar) -------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Story|Vitals") void SetDiseaseStage(EDiseaseStage Stage) { Disease = Stage; }
	/** Months since the cough began (Act 4 Phase 2) or weeks since diagnosis (Act 6). */
	UFUNCTION(BlueprintCallable, Category = "Story|Vitals") void SetDiseaseProgress(float MonthsSinceCough, float WeeksSinceDiagnosis);
	UFUNCTION(BlueprintCallable, Category = "Story|Vitals") void SetOnMedication(bool bOn) { bOnMedication = bOn; }

	// ---- health and injuries ------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Story|Vitals") void ApplyInjury(const FInjury& Injury, float HealthDamage);
	/** Call from landing: height in metres (Chaos ragdoll/capsule fall distance). */
	UFUNCTION(BlueprintCallable, Category = "Story|Vitals") void ApplyFall(float HeightMeters);
	/** Call at end of each in-game day (also heals injuries, applies hunger). */
	UFUNCTION(BlueprintCallable, Category = "Story|Vitals") void OnDayPassed(bool bSawDoctor);
	UFUNCTION(BlueprintPure, Category = "Story|Vitals") float GetHealth() const { return Health; }
	/** Movement multiplier from leg injuries (0.5 for a fresh baton knee). */
	UFUNCTION(BlueprintPure, Category = "Story|Vitals") float GetMoveSpeedMultiplier() const;

	// ---- hunger and heat ---------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Story|Vitals") void Eat() { HoursSinceMeal = 0.f; }
	UFUNCTION(BlueprintCallable, Category = "Story|Vitals") void SetHeatExposure(float DegreesC, bool bInShade);
	/** Advance hunger with the story clock (game hours). */
	UFUNCTION(BlueprintCallable, Category = "Story|Vitals") void AdvanceGameHours(float Hours) { HoursSinceMeal += Hours; }

	UPROPERTY(BlueprintAssignable, Category = "Story|Vitals") FOnCoughFit OnCoughFit;
	UPROPERTY(BlueprintAssignable, Category = "Story|Vitals") FOnVitalsDeath OnDeath;

	// ---- tuning --------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Vitals") float BaseMaxStamina = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Vitals") float RegenPerSecond = 9.f;
	/** Max-stamina lost per month of untreated cough, and per week after diagnosis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Vitals") float MaxLossPerMonth = 3.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Vitals") float MaxLossPerWeek = 1.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Vitals") float MinMaxStamina = 22.f;
	/** Below this fraction of max, coughing fits can trigger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Vitals") float CoughThreshold = 0.25f;

private:
	void MaybeCough(float DeltaTime);

	UPROPERTY(SaveGame) float Health = 100.f;
	UPROPERTY(SaveGame) float Stamina = 100.f;
	UPROPERTY(SaveGame) TArray<FInjury> Injuries;
	UPROPERTY(SaveGame) EDiseaseStage Disease = EDiseaseStage::Healthy;
	UPROPERTY(SaveGame) float MonthsCough = 0.f;
	UPROPERTY(SaveGame) float WeeksDiagnosed = 0.f;
	UPROPERTY(SaveGame) float HoursSinceMeal = 0.f;
	UPROPERTY(SaveGame) bool bOnMedication = false;

	float HeatDrainPerSecond = 0.f;
	float CoughLockRemaining = 0.f;
	float CoughCooldown = 0.f;
	FRandomStream Rng = FRandomStream(1988); // Daya's birth year
};
