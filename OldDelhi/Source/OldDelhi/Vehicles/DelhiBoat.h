#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DelhiBoat.generated.h"

class UStaticMeshComponent;
class UBoatBuoyancyComponent;

/**
 * Buoyancy for a rigid hull (Act 5 fishing boat). Several sample points on the hull each push up in proportion
 * to how deep they are below the water surface; the force per point is sized so the hull floats with the points
 * half submerged. Water height = flat level + optional two-wave swell. Needs no Water plugin; if the project
 * later uses one, override GetWaterHeightAt() or feed WaterLevelZCm from it.
 */
UCLASS(ClassGroup = (Physics), meta = (BlueprintSpawnableComponent))
class OLDDELHI_API UBoatBuoyancyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBoatBuoyancyComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Sample points in the root component's local space (cm). Defaults suit the placeholder 8 m hull. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy") TArray<FVector> BuoyancyPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy") float WaterLevelZCm = 0.f;
	/** Depth (cm) over which a point goes from dry to fully submerged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy") float SubmersionRangeCm = 60.f;
	/** Heave damping (1/s) relative to mass share; ~2 is near critical for a hull this size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy") float HeaveDamping = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves") bool bUseWaves = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves") float WaveAmplitudeCm = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves") float WaveLengthCm = 2500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves") float WavePeriodSeconds = 5.f;

	/** Water surface velocity (the ebb or flood). World km/h, horizontal. Default: 4 km/h ebb toward -X. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Current") FVector WaterCurrentKmh = FVector(-4.f, 0.f, 0.f);

	UFUNCTION(BlueprintPure, Category = "Buoyancy") float GetWaterHeightAt(const FVector& WorldLocation) const;
	/** Water surface velocity in cm/s. */
	UFUNCTION(BlueprintPure, Category = "Buoyancy") FVector GetWaterVelocityCmS() const { return WaterCurrentKmh * (100000.f / 3600.f); }
	/** Average submersion 0-1 of the sample points (0.5 = floating at design draft). */
	UFUNCTION(BlueprintPure, Category = "Buoyancy") float GetSubmergedFraction() const { return LastSubmerged; }

private:
	float LastSubmerged = 0.f;
	float TimeSeconds = 0.f;
};

UCLASS()
class OLDDELHI_API UDelhiBoatMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Time to get back to a point DistanceM behind the boat: turn around (TurnSeconds) then run at the boat's
	 * through-water speed with or against the current. Returns -1 if it can never make way (current >= boat speed).
	 * Physics alone: 25 km/h boat vs 4 km/h ebb is easily enough to return, so the bible's "the tide does not
	 * allow it" is a mission-layer rule: apply the 6-minute penalty (StoryTurnBackPenaltySeconds) there.
	 */
	UFUNCTION(BlueprintPure, Category = "Delhi|Boat")
	static float EstimateReturnSeconds(float DistanceM, float BoatSpeedKmh, float CurrentKmhAlongReturn, float TurnSeconds);
};

/**
 * Act 5 fishing boat: ~40 hp outboard, 25 km/h through the water at full throttle, flat-water hull on a physics
 * body. Placeholder hull is the engine cube (8 m x 2.2 m x 0.9 m); assign the real mesh in a Blueprint child and
 * re-place BuoyancyPoints. Possess it and call SetBoatInput(Throttle, Rudder) from the controller.
 */
UCLASS(Blueprintable)
class OLDDELHI_API ADelhiFishingBoat : public APawn
{
	GENERATED_BODY()

public:
	ADelhiFishingBoat();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat") TObjectPtr<UStaticMeshComponent> Hull;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boat") TObjectPtr<UBoatBuoyancyComponent> Buoyancy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat") float HullMassKg = 2500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat") float EnginePowerHp = 40.f;
	/** Propeller + gearing efficiency (shaft power to thrust power). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat") float PropEfficiency = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat") float TopSpeedKmh = 25.f;
	/** Static (bollard) thrust cap, N. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat") float MaxStaticThrustN = 3900.f;
	/** Lateral (keel) drag as a multiple of forward drag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat") float LateralDragFactor = 8.f;
	/** Stern offset (cm, hull local) where thrust and rudder act. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat") FVector SternOffsetCm = FVector(-380.f, 0.f, -40.f);
	/** Rudder: lift coefficient * 0.5 * rho * area, N per (m/s)^2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat") float RudderForcePerSpeedSq = 150.f;
	/** Mission-layer constant for Act 5 (bible: turning back costs 6 minutes the tide does not allow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Story") float StoryTurnBackPenaltySeconds = 360.f;

	/** Throttle -0.5..1 (reverse is half power), Rudder -1..1. */
	UFUNCTION(BlueprintCallable, Category = "Boat") void SetBoatInput(float Throttle, float Rudder);

	/** Speed through the water (what the engine fights), km/h. */
	UFUNCTION(BlueprintPure, Category = "Boat") float GetWaterSpeedKmh() const;
	/** Speed over ground, km/h. */
	UFUNCTION(BlueprintPure, Category = "Boat") float GetGroundSpeedKmh() const;
	UFUNCTION(BlueprintCallable, Category = "Boat") void SetCurrentKmh(FVector WorldCurrentKmh);

private:
	float ThrottleInput = 0.f;
	float RudderInput = 0.f;
};
