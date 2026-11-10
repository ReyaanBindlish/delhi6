#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DhCrowdInstanceRenderer.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * Renders the instanced tiers (LOD2 near ~150, LOD3 far up to the hard cap) as two instanced static mesh
 * components with a fixed slot pool, so slots never shift when agents come and go. Per-instance custom data:
 * 0..2 = cloth tint RGB, 3 = skin variation. The mesh is a person-height proxy today; swap for a VAT
 * (vertex-animation-texture) mesh in docs/CROWD.md "VAT upgrade" without touching any code.
 */
UCLASS(NotBlueprintable, Transient)
class OLDDELHI_API ADhCrowdInstanceRenderer : public AActor
{
	GENERATED_BODY()

public:
	ADhCrowdInstanceRenderer();

	/** Builds both components. ProxyHeightCm is only used to place a pivot-at-centre mesh (the cylinder fallback). */
	void Setup(UStaticMesh* Mesh, UMaterialInterface* Material, int32 NearCapacity, int32 FarCapacity, bool bPivotAtFeet, float ProxyHeightCm);

	int32 AllocSlot(bool bFar);
	void FreeSlot(bool bFar, int32 Slot);
	/** Pose in world space. Feet position, yaw degrees, uniform scale, gait phase (radians) for bobbing. */
	void SetSlot(bool bFar, int32 Slot, const FVector& FeetPos, float YawDeg, float Scale, float GaitPhase, bool bWalking);
	void SetSlotColor(bool bFar, int32 Slot, const FLinearColor& Tint, float Skin);
	/** Call once per tick after SetSlot calls. */
	void Flush();

	int32 GetUsed(bool bFar) const { return bFar ? FarUsed : NearUsed; }

private:
	UInstancedStaticMeshComponent* Comp(bool bFar) const { return bFar ? FarISM : NearISM; }
	FTransform ParkedTransform() const;

	UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> NearISM = nullptr;
	UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> FarISM = nullptr;

	TArray<int32> FreeNear;
	TArray<int32> FreeFar;
	int32 NearUsed = 0;
	int32 FarUsed = 0;
	bool bDirtyNear = false;
	bool bDirtyFar = false;
	bool bPivotFeet = false;
	float ProxyHeight = 170.f;
};
