#pragma once

#include "CoreMinimal.h"
#include "ChaosVehicleWheel.h"
#include "DelhiVehicleWheels.generated.h"

/**
 * Wheel classes for the Old Delhi vehicles. Each is configured in its constructor (Chaos reads the wheel CDO at
 * vehicle creation). Numbers: docs/PHYSICS.md section 4. Units: cm, kg, N*m; spring rate N/cm.
 */

// ---- three-wheelers (e-rickshaw, cycle-rickshaw): one steered front wheel, two rear wheels ------------------
UCLASS()
class OLDDELHI_API UDelhiRickshawFrontWheel : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UDelhiRickshawFrontWheel();
};

UCLASS()
class OLDDELHI_API UDelhiRickshawRearWheel : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UDelhiRickshawRearWheel();
};

/** Cycle-rickshaw wheels are thin and lightly braked. */
UCLASS()
class OLDDELHI_API UDelhiCycleRickshawFrontWheel : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UDelhiCycleRickshawFrontWheel();
};

UCLASS()
class OLDDELHI_API UDelhiCycleRickshawRearWheel : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UDelhiCycleRickshawRearWheel();
};

// ---- motorbike: front, rear, and two hidden outriggers that only touch down when leaning too far -----------
UCLASS()
class OLDDELHI_API UDelhiBikeFrontWheel : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UDelhiBikeFrontWheel();
};

UCLASS()
class OLDDELHI_API UDelhiBikeRearWheel : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UDelhiBikeRearWheel();
};

UCLASS()
class OLDDELHI_API UDelhiBikeOutriggerWheel : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UDelhiBikeOutriggerWheel();
};

// ---- Fortuner-class SUV -----------------------------------------------------------------------------------
UCLASS()
class OLDDELHI_API UDelhiSuvFrontWheel : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UDelhiSuvFrontWheel();
};

UCLASS()
class OLDDELHI_API UDelhiSuvRearWheel : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UDelhiSuvRearWheel();
};
