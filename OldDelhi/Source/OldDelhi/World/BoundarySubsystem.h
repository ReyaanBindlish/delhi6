#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BoundarySubsystem.generated.h"

class AWorldBoundary;
class UMaterialParameterCollection;

/** Proximity in 0..1 (1 = touching the wall) and distance to the wall face in cm. For MetaSound hum / bit-crush and UI-free feedback. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBoundaryProximity, float, Proximity01, float, DistanceCm);
/** One-off glitch sting moment (player came within TouchDistance of the wall). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBoundaryTouched, AActor*, Pawn, FVector, ClosestPoint);

/**
 * Runs at 20 Hz (implementation.md 4.2). For every local player:
 *  - writes MPC_Boundary (PlayerPos, Proximity, RevealRadius) so M_Boundary reveals only the nearby patch,
 *  - computes distance to the nearest AWorldBoundary,
 *  - applies a gentle push back towards the city inside PushStartDistance (characters: velocity change;
 *    physics vehicles: impulse scaled by mass and speed),
 *  - fires OnBoundaryProximity every tick, and OnBoundaryTouched + controller rumble once per approach at TouchDistance,
 *  - drives each boundary's hum and proximity post-process.
 */
UCLASS()
class OLDDELHI_API UBoundarySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UBoundarySubsystem, STATGROUP_Tickables); }

	void RegisterBoundary(AWorldBoundary* Boundary);
	void UnregisterBoundary(AWorldBoundary* Boundary);

	/** Collection written each update (auto-loaded from /Game/Delhi/Materials/MPC_Boundary when empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	TObjectPtr<UMaterialParameterCollection> BoundaryCollection;

	/** Radius of the revealed lattice patch around the player (cm). Spec: 12 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	float RevealRadiusCm = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	float UpdateHz = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	bool bEnablePushBack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	bool bEnableRumble = true;

	UPROPERTY(BlueprintAssignable, Category = "Boundary") FOnBoundaryProximity OnBoundaryProximity;
	UPROPERTY(BlueprintAssignable, Category = "Boundary") FOnBoundaryTouched OnBoundaryTouched;

	/** Latest proximity 0..1 for the first local player (0 = far). */
	UFUNCTION(BlueprintPure, Category = "Boundary") float GetProximity() const { return LastProximity; }

private:
	void Update(float Dt);

	TArray<TWeakObjectPtr<AWorldBoundary>> Boundaries;
	TSet<TWeakObjectPtr<AActor>> TouchedPawns;
	float Accum = 0.f;
	float LastProximity = 0.f;
	bool bCollectionSearched = false;
};
