#include "Vehicles/DelhiVehiclePawn.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Physics/DelhiSurfaceLibrary.h"
#include "Vehicles/DelhiVehicleWheels.h"

// ------------------------------------------------------------------------------------------------- math
float UDelhiVehicleMath::StoppingDistanceM(float SpeedKmh, float DecelMS2)
{
	const float V = FMath::Max(SpeedKmh, 0.f) / 3.6f;
	return (V * V) / (2.f * FMath::Max(DecelMS2, 0.1f));
}

float UDelhiVehicleMath::FrictionLimitedDecel(float Mu)
{
	return FMath::Max(Mu, 0.f) * 9.81f;
}

float UDelhiVehicleMath::TotalBrakeTorqueForDecel(float MassKg, float WheelRadiusCm, float DecelMS2)
{
	return MassKg * DecelMS2 * (WheelRadiusCm * 0.01f);
}

// ------------------------------------------------------------------------------------------------- base
ADelhiVehiclePawn::ADelhiVehiclePawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

UChaosWheeledVehicleMovementComponent* ADelhiVehiclePawn::GetWheeledMovement() const
{
	return Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent());
}

void ADelhiVehiclePawn::AddWheel(FName Bone, TSubclassOf<UChaosVehicleWheel> WheelClass, const FVector& Offset)
{
	if (UChaosWheeledVehicleMovementComponent* M = GetWheeledMovement())
	{
		FChaosWheelSetup Setup;
		Setup.WheelClass = WheelClass;
		Setup.BoneName = Bone;
		Setup.AdditionalOffset = Offset;
		M->WheelSetups.Add(Setup);
	}
}

void ADelhiVehiclePawn::SetupPowertrain(float MaxTorqueNm, float MaxRPM, float IdleRPM, float FinalRatio, bool bRearDrive,
                                        const TArray<float>& ForwardGears)
{
	UChaosWheeledVehicleMovementComponent* M = GetWheeledMovement();
	if (!M)
	{
		return;
	}
	M->EngineSetup.MaxTorque = MaxTorqueNm;
	M->EngineSetup.MaxRPM = MaxRPM;
	M->EngineSetup.EngineIdleRPM = IdleRPM;

	M->TransmissionSetup.FinalRatio = FinalRatio;
	M->TransmissionSetup.ForwardGearRatios = ForwardGears;
	M->TransmissionSetup.ReverseGearRatios.Reset();
	M->TransmissionSetup.ReverseGearRatios.Add(ForwardGears.Num() > 0 ? ForwardGears[0] : 1.f);
	M->TransmissionSetup.ChangeUpRPM = MaxRPM * 0.8f;
	M->TransmissionSetup.ChangeDownRPM = MaxRPM * 0.35f;

	M->DifferentialSetup.DifferentialType = bRearDrive ? EVehicleDifferential::RearWheelDrive : EVehicleDifferential::AllWheelDrive;
}

void ADelhiVehiclePawn::SetupChassis(float HeightCm, float WidthCm, float DragCoeff)
{
	if (UChaosWheeledVehicleMovementComponent* M = GetWheeledMovement())
	{
		M->ChassisHeight = HeightCm;
		M->ChassisWidth = WidthCm;
		M->DragCoefficient = DragCoeff;
		M->bEnableCenterOfMassOverride = true;
	}
	ApplyMassAndCom();
}

float ADelhiVehiclePawn::GetTotalMassKg() const
{
	return Profile.EmptyMassKg + Profile.PassengerMassKg * Passengers;
}

void ADelhiVehiclePawn::ApplyMassAndCom()
{
	UChaosWheeledVehicleMovementComponent* M = GetWheeledMovement();
	if (!M)
	{
		return;
	}
	const float Total = GetTotalMassKg();
	const float PaxMass = Profile.PassengerMassKg * Passengers;
	const float ComZ = (Profile.EmptyMassKg * Profile.EmptyComHeightCm + PaxMass * Profile.PassengerSeatHeightCm) / FMath::Max(Total, 1.f);
	M->Mass = Total;
	M->CenterOfMassOverride = FVector(Profile.ComForwardCm, 0.f, ComZ);
}

void ADelhiVehiclePawn::SetPassengerCount(int32 Count)
{
	Passengers = FMath::Clamp(Count, 0, Profile.MaxPassengers);
	ApplyMassAndCom();
	if (HasActorBegunPlay())
	{
		if (UChaosWheeledVehicleMovementComponent* M = GetWheeledMovement())
		{
			M->RecreatePhysicsState(); // pick up mass and COM (do this while stationary)
		}
	}
}

void ADelhiVehiclePawn::BeginPlay()
{
	Super::BeginPlay();
	ApplyMassAndCom();
}

void ADelhiVehiclePawn::SetDriveInput(float Throttle, float Brake, float Steer, bool bHandbrake)
{
	if (UChaosWheeledVehicleMovementComponent* M = GetWheeledMovement())
	{
		LastSteerInput = FMath::Clamp(Steer, -1.f, 1.f);
		M->SetThrottleInput(FMath::Clamp(Throttle, 0.f, 1.f));
		M->SetBrakeInput(FMath::Clamp(Brake, 0.f, 1.f));
		M->SetSteeringInput(LastSteerInput);
		M->SetHandbrakeInput(bHandbrake);
	}
}

float ADelhiVehiclePawn::GetSpeedKmh() const
{
	return FVector::DotProduct(GetVelocity(), GetActorForwardVector()) * 0.036f; // cm/s -> km/h
}

float ADelhiVehiclePawn::EstimateStoppingDistanceM() const
{
	const float Mu = UDelhiSurfaceLibrary::GetEffectiveFriction(this, EDelhiSurface::Asphalt);
	const float Decel = FMath::Min(Profile.DesignDecelMS2, UDelhiVehicleMath::FrictionLimitedDecel(Mu));
	return UDelhiVehicleMath::StoppingDistanceM(FMath::Abs(GetSpeedKmh()), Decel);
}

float ADelhiVehiclePawn::GetRolloverThresholdG() const
{
	const float Total = GetTotalMassKg();
	const float ComZ = (Profile.EmptyMassKg * Profile.EmptyComHeightCm + Profile.PassengerMassKg * Passengers * Profile.PassengerSeatHeightCm) / FMath::Max(Total, 1.f);
	return (Profile.TrackWidthCm * 0.5f) / FMath::Max(ComZ, 1.f);
}

void ADelhiVehiclePawn::StartBrakeTest(float SpeedKmh)
{
	bBrakeTestActive = true;
	BrakeTestIdleTime = 0.f;
	BrakeTestStart = GetActorLocation();
	GetMesh()->SetPhysicsLinearVelocity(GetActorForwardVector() * (SpeedKmh / 0.036f));
}

void ADelhiVehiclePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UChaosWheeledVehicleMovementComponent* M = GetWheeledMovement();
	if (!M)
	{
		return;
	}

	// Weather: wet asphalt lowers wheel friction (re-evaluated twice a second, it changes slowly).
	WeatherAccumulator += DeltaSeconds;
	if (bApplyWeatherFriction && WeatherAccumulator >= 0.5f)
	{
		WeatherAccumulator = 0.f;
		WeatherFrictionScale = UDelhiSurfaceLibrary::GetWetFrictionScale(this, EDelhiSurface::Asphalt);
		if (BaseWheelFriction.Num() != M->Wheels.Num())
		{
			BaseWheelFriction.Reset();
			for (UChaosVehicleWheel* W : M->Wheels)
			{
				BaseWheelFriction.Add(W ? W->FrictionForceMultiplier : 1.f);
			}
		}
		for (int32 i = 0; i < M->Wheels.Num(); ++i)
		{
			if (M->Wheels[i] && BaseWheelFriction.IsValidIndex(i))
			{
				M->Wheels[i]->FrictionForceMultiplier = BaseWheelFriction[i] * WeatherFrictionScale;
			}
		}
	}

	if (bBrakeTestActive)
	{
		M->SetThrottleInput(0.f);
		M->SetBrakeInput(1.f);
		const float SpeedMS = GetVelocity().Size() * 0.01f;
		BrakeTestIdleTime = SpeedMS < 0.3f ? BrakeTestIdleTime + DeltaSeconds : 0.f;
		if (BrakeTestIdleTime > 0.25f)
		{
			bBrakeTestActive = false;
			M->SetBrakeInput(0.f);
			LastBrakeTestM = FVector::Dist(BrakeTestStart, GetActorLocation()) * 0.01f;
			UE_LOG(LogTemp, Log, TEXT("[Vehicle] %s brake test stopped in %.1f m"), *GetName(), LastBrakeTestM);
		}
	}
}

// ------------------------------------------------------------------------------------------------- concrete
ADelhiERickshawPawn::ADelhiERickshawPawn(const FObjectInitializer& OI) : Super(OI)
{
	Profile.DisplayName = TEXT("E-Rickshaw");
	Profile.EmptyMassKg = 350.f;
	Profile.PassengerMassKg = 70.f;
	Profile.MaxPassengers = 4;
	Profile.EmptyComHeightCm = 55.f;   // high: battery pack under the seat, open canopy
	Profile.PassengerSeatHeightCm = 85.f;
	Profile.TrackWidthCm = 100.f;
	Profile.DesignDecelMS2 = 3.5f;
	Profile.TopSpeedKmh = 25.f;

	AddWheel(TEXT("Wheel_F"), UDelhiRickshawFrontWheel::StaticClass());
	AddWheel(TEXT("Wheel_RL"), UDelhiRickshawRearWheel::StaticClass());
	AddWheel(TEXT("Wheel_RR"), UDelhiRickshawRearWheel::StaticClass());
	SetupPowertrain(14.f, 2300.f, 50.f, 9.f, true, { 1.f });
	SetupChassis(170.f, 100.f, 0.9f);
}

ADelhiCycleRickshawPawn::ADelhiCycleRickshawPawn(const FObjectInitializer& OI) : Super(OI)
{
	Profile.DisplayName = TEXT("Cycle-Rickshaw");
	Profile.EmptyMassKg = 160.f;       // frame + rider
	Profile.PassengerMassKg = 70.f;
	Profile.MaxPassengers = 2;
	Profile.EmptyComHeightCm = 60.f;
	Profile.PassengerSeatHeightCm = 75.f;
	Profile.TrackWidthCm = 105.f;
	Profile.DesignDecelMS2 = 2.5f;
	Profile.TopSpeedKmh = 12.f;

	AddWheel(TEXT("Wheel_F"), UDelhiCycleRickshawFrontWheel::StaticClass());
	AddWheel(TEXT("Wheel_RL"), UDelhiCycleRickshawRearWheel::StaticClass());
	AddWheel(TEXT("Wheel_RR"), UDelhiCycleRickshawRearWheel::StaticClass());
	// Pedal limited: ~150 W sustained -> low torque, RPM cap gives ~12 km/h.
	SetupPowertrain(7.f, 520.f, 20.f, 5.f, true, { 1.f });
	SetupChassis(160.f, 105.f, 0.9f);
}

ADelhiSuvPawn::ADelhiSuvPawn(const FObjectInitializer& OI) : Super(OI)
{
	Profile.DisplayName = TEXT("Fortuner-class SUV");
	Profile.EmptyMassKg = 2100.f;
	Profile.PassengerMassKg = 75.f;
	Profile.MaxPassengers = 6;
	Profile.EmptyComHeightCm = 75.f;
	Profile.PassengerSeatHeightCm = 95.f;
	Profile.TrackWidthCm = 157.f;
	Profile.DesignDecelMS2 = 6.17f;    // 80 km/h in 40 m: v^2 / 2d = 22.2^2 / 80
	Profile.TopSpeedKmh = 170.f;

	AddWheel(TEXT("Wheel_FL"), UDelhiSuvFrontWheel::StaticClass());
	AddWheel(TEXT("Wheel_FR"), UDelhiSuvFrontWheel::StaticClass());
	AddWheel(TEXT("Wheel_RL"), UDelhiSuvRearWheel::StaticClass());
	AddWheel(TEXT("Wheel_RR"), UDelhiSuvRearWheel::StaticClass());
	SetupPowertrain(420.f, 4000.f, 800.f, 3.9f, true, { 3.52f, 2.04f, 1.40f, 1.00f, 0.71f, 0.58f });
	SetupChassis(180.f, 190.f, 0.40f);
}

ADelhiMotorbikePawn::ADelhiMotorbikePawn(const FObjectInitializer& OI) : Super(OI)
{
	Profile.DisplayName = TEXT("Motorbike");
	Profile.EmptyMassKg = 265.f;       // 190 kg bike + 75 kg rider
	Profile.PassengerMassKg = 65.f;
	Profile.MaxPassengers = 1;
	Profile.EmptyComHeightCm = 60.f;
	Profile.PassengerSeatHeightCm = 85.f;
	Profile.TrackWidthCm = 70.f;       // outrigger span
	Profile.DesignDecelMS2 = 5.5f;
	Profile.TopSpeedKmh = 110.f;

	AddWheel(TEXT("Wheel_F"), UDelhiBikeFrontWheel::StaticClass());
	AddWheel(TEXT("Wheel_R"), UDelhiBikeRearWheel::StaticClass());
	AddWheel(TEXT("Outrigger_L"), UDelhiBikeOutriggerWheel::StaticClass(), FVector(0.f, 0.f, 15.f));
	AddWheel(TEXT("Outrigger_R"), UDelhiBikeOutriggerWheel::StaticClass(), FVector(0.f, 0.f, 15.f));
	SetupPowertrain(24.f, 5500.f, 1100.f, 6.0f, true, { 2.8f, 1.9f, 1.4f, 1.1f, 0.9f });
	SetupChassis(110.f, 70.f, 0.6f);
}

void ADelhiMotorbikePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StepLean(DeltaSeconds);
}

void ADelhiMotorbikePawn::StepLean(float Dt)
{
	USkeletalMeshComponent* Mesh = GetMesh();
	if (!Mesh || Dt <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	// Roll, positive = leaning right (right vector points down).
	const float Roll = FMath::Asin(FMath::Clamp(-GetActorRightVector().Z, -1.f, 1.f));
	const float RollRate = (Roll - LastLeanRad) / Dt;
	LastLeanRad = Roll;
	CurrentLeanDeg = FMath::RadiansToDegrees(Roll);

	const float SpeedKmh = FMath::Abs(GetSpeedKmh());
	const float SpeedFactor = FMath::Clamp((SpeedKmh - LowSpeedKmhNoLean) / 30.f, 0.f, 1.f);
	const float MaxLeanRad = FMath::DegreesToRadians(MaxLeanDeg);
	// Bank into the turn: a = v^2/R; lean angle = atan(a/g). Steer input stands in for 1/R.
	const float Target = FMath::Clamp((LastSteerInput * SpeedFactor + RiderLeanInput) * MaxLeanRad, -MaxLeanRad, MaxLeanRad);

	const float Accel = LeanKp * (Target - Roll) - LeanKd * RollRate; // rad/s^2 about the forward axis
	Mesh->AddTorqueInRadians(GetActorForwardVector() * (Accel * LeanTorqueSign), NAME_None, /*bAccelChange*/ true);
}
