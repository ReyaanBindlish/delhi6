#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"
#include "Math/RandomStream.h"
#include "Physics/DelhiSurfaceTypes.h"
#include "DelhiBallisticsLibrary.generated.h"

/**
 * Ballistics helper (library only, no weapon system yet). Bible rules:
 *  - bullet drop (segmented gravity arc), penetration by material (brick stops a 9 mm, a car door does not)
 *  - NO aim assist: the shot direction is exactly what the caller passes, plus the weapon's own spread cone
 *  - a katta misfires 1 time in 8
 * Actors tagged "BallisticsIgnore" (the construct wall) are skipped.
 */
UENUM(BlueprintType)
enum class EDelhiWeaponClass : uint8
{
	Pistol9mm,
	Katta,
	Rifle762
};

UENUM(BlueprintType)
enum class EDelhiImpactResult : uint8
{
	Stopped,
	Penetrated,
	Ricocheted
};

USTRUCT(BlueprintType)
struct OLDDELHI_API FDelhiBallisticProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") EDelhiWeaponClass WeaponClass = EDelhiWeaponClass::Pistol9mm;
	/** Muzzle energy in joules (9 mm ~500 J, katta ~250 J, 7.62 ~2500 J). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") float MuzzleEnergyJ = 500.f;
	/** cm/s (9 mm ~ 350 m/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") float MuzzleVelocityCmS = 35000.f;
	/** Half-angle of the random spread cone in degrees. Never narrowed by any assist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") float SpreadHalfAngleDeg = 0.6f;
	/** 0 = never misfires; 8 = one in eight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") int32 MisfireOneIn = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") float MaxRangeCm = 25000.f;
	/** Length of each straight trace segment of the arc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") float SegmentLengthCm = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") float MinEnergyJ = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") int32 MaxImpacts = 6;
	/** Thickness search distance behind a hit (cm) when estimating how much material the bullet crosses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") float MaxThicknessCm = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistics") TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

USTRUCT(BlueprintType)
struct OLDDELHI_API FDelhiBulletImpact
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics") FHitResult Hit;
	UPROPERTY(BlueprintReadOnly, Category = "Ballistics") EDelhiSurface Surface = EDelhiSurface::Default;
	UPROPERTY(BlueprintReadOnly, Category = "Ballistics") EDelhiImpactResult Result = EDelhiImpactResult::Stopped;
	UPROPERTY(BlueprintReadOnly, Category = "Ballistics") float EnergyBeforeJ = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Ballistics") float EnergyAfterJ = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Ballistics") float ThicknessCm = 0.f;
};

USTRUCT(BlueprintType)
struct OLDDELHI_API FDelhiShotResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics") bool bMisfired = false;
	UPROPERTY(BlueprintReadOnly, Category = "Ballistics") TArray<FDelhiBulletImpact> Impacts;
	/** Where the bullet ended (last impact, or max range). */
	UPROPERTY(BlueprintReadOnly, Category = "Ballistics") FVector EndPoint = FVector::ZeroVector;
};

UCLASS()
class OLDDELHI_API UDelhiBallisticsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Delhi|Ballistics")
	static FDelhiBallisticProfile GetProfile(EDelhiWeaponClass WeaponClass);

	/** True when the weapon misfires on this trigger pull. Seed the stream per weapon for reproducible saves. */
	UFUNCTION(BlueprintCallable, Category = "Delhi|Ballistics")
	static bool RollMisfire(const FDelhiBallisticProfile& Profile, UPARAM(ref) FRandomStream& Stream);

	/**
	 * Fire one bullet. Returns false on a misfire. Traces a gravity arc in segments, applies material
	 * penetration (energy loss = surface J/cm * thickness) and ricochet at shallow angles.
	 * Does NOT apply damage; the caller reads OutResult.Impacts (actor, bone, surface, energy).
	 */
	UFUNCTION(BlueprintCallable, Category = "Delhi|Ballistics", meta = (WorldContext = "WorldContextObject"))
	static bool FireShot(const UObject* WorldContextObject, AActor* Instigator, FVector Origin, FVector AimDirection,
	                     const FDelhiBallisticProfile& Profile, UPARAM(ref) FRandomStream& Stream,
	                     FDelhiShotResult& OutResult, bool bDrawDebug = false);

	/** Vertical drop (cm, positive) at a range, ignoring penetration. For HUD-free sight tables and tests. */
	UFUNCTION(BlueprintPure, Category = "Delhi|Ballistics", meta = (WorldContext = "WorldContextObject"))
	static float GetDropAtRangeCm(const UObject* WorldContextObject, const FDelhiBallisticProfile& Profile, float RangeCm);

	/** Would a bullet with this energy be stopped by this thickness of this surface? (No ricochet.) */
	UFUNCTION(BlueprintPure, Category = "Delhi|Ballistics")
	static bool WouldStop(const FDelhiBallisticProfile& Profile, EDelhiSurface Surface, float ThicknessCm);
};
