#include "Vehicles/DelhiVehicleWheels.h"

namespace
{
	struct FWheelParams
	{
		float RadiusCm;
		float WidthCm;
		float MassKg;
		float Friction;        // FrictionForceMultiplier
		bool bSteer;
		float MaxSteerDeg;
		bool bBrake;
		bool bHandbrake;
		bool bEngine;
		float BrakeNm;
		float HandbrakeNm;
		float SpringRate;      // N/cm
		float DampingRatio;
		float MaxRaiseCm;
		float MaxDropCm;
		bool bFrontAxle;
	};

	void Configure(UChaosVehicleWheel* W, const FWheelParams& P)
	{
		W->WheelRadius = P.RadiusCm;
		W->WheelWidth = P.WidthCm;
		W->WheelMass = P.MassKg;
		W->FrictionForceMultiplier = P.Friction;
		W->bAffectedBySteering = P.bSteer;
		W->MaxSteerAngle = P.MaxSteerDeg;
		W->bAffectedByBrake = P.bBrake;
		W->bAffectedByHandbrake = P.bHandbrake;
		W->bAffectedByEngine = P.bEngine;
		W->MaxBrakeTorque = P.BrakeNm;
		W->MaxHandBrakeTorque = P.HandbrakeNm;
		W->SpringRate = P.SpringRate;
		W->SuspensionDampingRatio = P.DampingRatio;
		W->SuspensionMaxRaise = P.MaxRaiseCm;
		W->SuspensionMaxDrop = P.MaxDropCm;
		W->AxleType = P.bFrontAxle ? EAxleType::Front : EAxleType::Rear;
	}
}

// radius, width, mass, friction, steer, maxSteer, brake, handbrake, engine, brakeNm, handNm, spring, damp, raise, drop, front
UDelhiRickshawFrontWheel::UDelhiRickshawFrontWheel()
{
	// 3.50-12 tyre, steered single front wheel, drum brake (weak).
	Configure(this, { 28.f, 11.f, 6.f, 1.0f, true, 40.f, true, false, false, 150.f, 0.f, 90.f, 0.55f, 6.f, 8.f, true });
}

UDelhiRickshawRearWheel::UDelhiRickshawRearWheel()
{
	Configure(this, { 28.f, 11.f, 7.f, 1.0f, false, 0.f, true, true, true, 220.f, 350.f, 120.f, 0.55f, 6.f, 8.f, false });
}

UDelhiCycleRickshawFrontWheel::UDelhiCycleRickshawFrontWheel()
{
	Configure(this, { 33.f, 4.f, 3.f, 0.9f, true, 35.f, true, false, false, 80.f, 0.f, 70.f, 0.5f, 5.f, 6.f, true });
}

UDelhiCycleRickshawRearWheel::UDelhiCycleRickshawRearWheel()
{
	Configure(this, { 33.f, 4.f, 3.5f, 0.9f, false, 0.f, true, true, true, 80.f, 120.f, 90.f, 0.5f, 5.f, 6.f, false });
}

UDelhiBikeFrontWheel::UDelhiBikeFrontWheel()
{
	Configure(this, { 31.f, 10.f, 7.f, 1.0f, true, 28.f, true, false, false, 300.f, 0.f, 110.f, 0.6f, 8.f, 10.f, true });
}

UDelhiBikeRearWheel::UDelhiBikeRearWheel()
{
	Configure(this, { 31.f, 12.f, 8.f, 1.0f, false, 0.f, true, true, true, 150.f, 250.f, 130.f, 0.6f, 8.f, 10.f, false });
}

UDelhiBikeOutriggerWheel::UDelhiBikeOutriggerWheel()
{
	// Small, free-rolling, low-friction, mounted ~15 cm up each side of the rear axle: only touches the road past ~25 degrees lean.
	Configure(this, { 12.f, 4.f, 1.f, 0.4f, false, 0.f, false, false, false, 0.f, 0.f, 40.f, 0.8f, 4.f, 10.f, false });
}

UDelhiSuvFrontWheel::UDelhiSuvFrontWheel()
{
	// 265/65 R17 (~37.8 cm radius). Brake torque chosen so ABS-limited stop from 80 km/h is ~40 m (see docs).
	Configure(this, { 37.8f, 26.5f, 22.f, 1.0f, true, 35.f, true, false, false, 1500.f, 0.f, 350.f, 0.6f, 12.f, 14.f, true });
}

UDelhiSuvRearWheel::UDelhiSuvRearWheel()
{
	Configure(this, { 37.8f, 26.5f, 22.f, 1.0f, false, 0.f, true, true, true, 1000.f, 2500.f, 350.f, 0.6f, 12.f, 14.f, false });
}
