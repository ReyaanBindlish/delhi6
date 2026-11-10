#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WheeledVehiclePawn.h"
#include "ChaosVehicleWheel.h"
#include "DelhiVehiclePawn.generated.h"

class UChaosWheeledVehicleMovementComponent;

/** Pure vehicle maths (C++ and Blueprint). */
UCLASS()
class OLDDELHI_API UDelhiVehicleMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** v^2 / (2a), metres. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Vehicle")
	static float StoppingDistanceM(float SpeedKmh, float DecelMS2);

	/** Deceleration limited by tyre friction (mu * g). */
	UFUNCTION(BlueprintPure, Category = "Delhi|Vehicle")
	static float FrictionLimitedDecel(float Mu);

	/** Total brake torque (Nm, sum over all braked wheels) that gives DecelMS2 for a mass and wheel radius (cm). */
	UFUNCTION(BlueprintPure, Category = "Delhi|Vehicle")
	static float TotalBrakeTorqueForDecel(float MassKg, float WheelRadiusCm, float DecelMS2);
};

/** Tunable per-vehicle numbers that are not Chaos wheel/engine settings. */
USTRUCT(BlueprintType)
struct OLDDELHI_API FDelhiVehicleProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") FName DisplayName = TEXT("Vehicle");
	/** Empty mass incl. driver. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") float EmptyMassKg = 350.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") float PassengerMassKg = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") int32 MaxPassengers = 3;
	/** COM height above the mesh origin (cm) empty / with a seated passenger; origin is assumed at ground level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") float EmptyComHeightCm = 55.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") float PassengerSeatHeightCm = 85.f;
	/** Forward/back COM offset (cm, + forward). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") float ComForwardCm = 0.f;
	/** Designed braking deceleration on dry asphalt (m/s^2); used for stopping-distance estimates and AI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") float DesignDecelMS2 = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") float TopSpeedKmh = 25.f;
	/** Track width (cm): drives the rollover threshold = (track/2)/comHeight in g. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") float TrackWidthCm = 100.f;
};

/**
 * Base class for all Old Delhi Chaos vehicles. Concrete classes set the wheel setups and powertrain in their
 * constructors (Chaos needs them before physics state creation). Assign a skeletal mesh + Physics Asset +
 * Anim BP (UChaosVehicleAnimationInstance) in a Blueprint child. Default bone names are documented per class.
 *
 * Input is deliberately engine-agnostic: call SetDriveInput() from the player controller (keyboard, gamepad,
 * touch; whoever owns the controls). Weather: every 0.5 s wheel friction is scaled by wet asphalt
 * (UDelhiSurfaceLibrary), and the brake is limited accordingly.
 */
UCLASS(Abstract)
class OLDDELHI_API ADelhiVehiclePawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
	ADelhiVehiclePawn(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") FDelhiVehicleProfile Profile;
	/** Assumed surface when the weather scales friction (roads are asphalt). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle") bool bApplyWeatherFriction = true;

	/** Throttle/Brake 0-1, Steer -1..1, Handbrake bool. Throttle with reverse: pass Brake while stopped = reverse handled by Chaos auto-reverse. */
	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	void SetDriveInput(float Throttle, float Brake, float Steer, bool bHandbrake);

	UFUNCTION(BlueprintPure, Category = "Vehicle") float GetSpeedKmh() const;
	/** Estimated stopping distance from the current speed on the current (wet/dry) road. */
	UFUNCTION(BlueprintPure, Category = "Vehicle") float EstimateStoppingDistanceM() const;
	/** Lateral acceleration (g) at which this vehicle starts to tip: (track/2)/COM height. */
	UFUNCTION(BlueprintPure, Category = "Vehicle") float GetRolloverThresholdG() const;
	UFUNCTION(BlueprintPure, Category = "Vehicle") float GetTotalMassKg() const;

	/** Adds seated passengers: raises mass and COM (a loaded e-rickshaw tips more easily). */
	UFUNCTION(BlueprintCallable, Category = "Vehicle") void SetPassengerCount(int32 Count);
	UFUNCTION(BlueprintPure, Category = "Vehicle") int32 GetPassengerCount() const { return Passengers; }

	/** Test: launch at SpeedKmh, full brake, log the stopping distance. Console: use from a test Blueprint/automation. */
	UFUNCTION(BlueprintCallable, Category = "Vehicle|Test") void StartBrakeTest(float SpeedKmh = 80.f);
	UFUNCTION(BlueprintPure, Category = "Vehicle|Test") float GetLastBrakeTestDistanceM() const { return LastBrakeTestM; }

protected:
	UChaosWheeledVehicleMovementComponent* GetWheeledMovement() const;
	/** Derived constructors call these. */
	void AddWheel(FName Bone, TSubclassOf<UChaosVehicleWheel> WheelClass, const FVector& Offset = FVector::ZeroVector);
	void SetupPowertrain(float MaxTorqueNm, float MaxRPM, float IdleRPM, float FinalRatio, bool bRearDrive,
	                     const TArray<float>& ForwardGears);
	void SetupChassis(float HeightCm, float WidthCm, float DragCoeff);
	void ApplyMassAndCom();

	int32 Passengers = 0;
	float LastSteerInput = 0.f;
	float WeatherAccumulator = 0.f;
	float WeatherFrictionScale = 1.f;
	TArray<float> BaseWheelFriction;

	bool bBrakeTestActive = false;
	FVector BrakeTestStart = FVector::ZeroVector;
	float BrakeTestIdleTime = 0.f;
	float LastBrakeTestM = 0.f;
	float LastBrakeInput = 0.f;
};

// ------------------------------------------------------------------------------------------------------------
/** E-rickshaw: 3 wheels, ~350 kg, raised COM, 25 km/h. Bones: Wheel_F, Wheel_RL, Wheel_RR. */
UCLASS(Blueprintable)
class OLDDELHI_API ADelhiERickshawPawn : public ADelhiVehiclePawn
{
	GENERATED_BODY()
public:
	ADelhiERickshawPawn(const FObjectInitializer& ObjectInitializer);
};

/** Cycle-rickshaw: same 3-wheel rig, pedal-limited torque (~150 W), 12 km/h. Bones as above. */
UCLASS(Blueprintable)
class OLDDELHI_API ADelhiCycleRickshawPawn : public ADelhiVehiclePawn
{
	GENERATED_BODY()
public:
	ADelhiCycleRickshawPawn(const FObjectInitializer& ObjectInitializer);
};

/** Fortuner-class SUV: 2,100 kg, 4 wheels, stops from 80 km/h in ~40 m. Bones: Wheel_FL, Wheel_FR, Wheel_RL, Wheel_RR. */
UCLASS(Blueprintable)
class OLDDELHI_API ADelhiSuvPawn : public ADelhiVehiclePawn
{
	GENERATED_BODY()
public:
	ADelhiSuvPawn(const FObjectInitializer& ObjectInitializer);
};

/**
 * Motorbike: 4 simulated wheels (front, rear, two small outriggers that only touch past ~25 degrees lean) and a
 * PD lean torque so it banks into turns and does not fall over at rest. Bones: Wheel_F, Wheel_R, Outrigger_L, Outrigger_R.
 * Visual lean: read GetLeanAngleDeg() in the AnimBP / on a mesh pivot.
 */
UCLASS(Blueprintable)
class OLDDELHI_API ADelhiMotorbikePawn : public ADelhiVehiclePawn
{
	GENERATED_BODY()
public:
	ADelhiMotorbikePawn(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motorbike") float MaxLeanDeg = 35.f;
	/** Proportional / derivative gains of the lean controller (rad/s^2 per rad, rad/s^2 per rad/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motorbike") float LeanKp = 38.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motorbike") float LeanKd = 9.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motorbike") float LowSpeedKmhNoLean = 6.f;
	/** -1 or +1. Flip if the bike falls the wrong way in the first test (torque handedness in UE is easy to get backwards). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motorbike") float LeanTorqueSign = -1.f;
	/** Rider's lean command -1..1, added to the steering-derived lean. */
	UPROPERTY(BlueprintReadWrite, Category = "Motorbike") float RiderLeanInput = 0.f;

	UFUNCTION(BlueprintPure, Category = "Motorbike") float GetLeanAngleDeg() const { return CurrentLeanDeg; }

private:
	/** Called from Tick; keep the body so it can move to AsyncPhysicsTickActor once that API is confirmed. */
	void StepLean(float Dt);

	float CurrentLeanDeg = 0.f;
	float LastLeanRad = 0.f;
};
