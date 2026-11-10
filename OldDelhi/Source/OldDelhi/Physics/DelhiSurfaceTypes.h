#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "DelhiSurfaceTypes.generated.h"

/**
 * Physical surface types for Old Delhi (implementation.md 4.1, research/03).
 *
 * The numeric value of each entry is IDENTICAL to the EPhysicalSurface value, so
 * static_cast<EPhysicalSurface>(EDelhiSurface::LakhoriBrick) == SurfaceType1.
 * Project Settings > Physics > Physical Surfaces must list them in this order
 * (see Config lines in docs/PHYSICS.md; Tools/ue/setup_physics.py creates the assets).
 */
UENUM(BlueprintType)
enum class EDelhiSurface : uint8
{
	Default            = 0 UMETA(DisplayName = "Default"),
	LakhoriBrick       = 1 UMETA(DisplayName = "Lakhori Brick"),
	LimePlaster        = 2 UMETA(DisplayName = "Lime Plaster"),
	RedSandstone       = 3 UMETA(DisplayName = "Red Sandstone"),
	KotaStone          = 4 UMETA(DisplayName = "Kota Stone"),
	CorrugatedShutter  = 5 UMETA(DisplayName = "Corrugated Shutter"),
	Glass              = 6 UMETA(DisplayName = "Glass"),
	Asphalt            = 7 UMETA(DisplayName = "Asphalt"),
	Mud                = 8 UMETA(DisplayName = "Mud / Slush"),
	Wood               = 9 UMETA(DisplayName = "Wood"),
	SheetMetal         = 10 UMETA(DisplayName = "Sheet Metal (car body)"),
	Water              = 11 UMETA(DisplayName = "Water"),
	Concrete           = 12 UMETA(DisplayName = "Concrete"),
	Flesh              = 13 UMETA(DisplayName = "Flesh (characters)"),
	Count              = 14 UMETA(Hidden)
};

/** Per-surface physical and ballistic data. Static table lives in DelhiSurfaceLibrary.cpp. */
USTRUCT(BlueprintType)
struct OLDDELHI_API FDelhiSurfaceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface") FName SurfaceName = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface") float Friction = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface") float Restitution = 0.1f;
	/** g/cm^3 (physical material Density field uses the same unit). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface") float DensityGCm3 = 2.0f;
	/** Friction multiplier when MPC_Weather Wetness = 1 (1.0 = unaffected). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface") float WetFrictionScale = 0.85f;
	/** True for mud: Friction Combine = Multiply so vehicles slide. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface") bool bMultiplyCombine = false;

	/** Energy absorbed per cm of material thickness (J/cm). 9 mm = ~500 J, katta = ~250 J. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") float PenetrationJPerCm = 60.f;
	/** Bullets striking at less than this angle from the surface ricochet (0 = never). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") float RicochetAngleDeg = 0.f;
};
