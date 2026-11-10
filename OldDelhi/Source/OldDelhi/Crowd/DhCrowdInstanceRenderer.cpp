#include "Crowd/DhCrowdInstanceRenderer.h"

#include "Algo/Reverse.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ADhCrowdInstanceRenderer::ADhCrowdInstanceRenderer()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	NearISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("NearISM"));
	FarISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FarISM"));
	for (UInstancedStaticMeshComponent* C : { NearISM.Get(), FarISM.Get() })
	{
		C->SetupAttachment(Root);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		C->NumCustomDataFloats = 4;
		C->bAffectDistanceFieldLighting = false;
	}
	NearISM->SetCastShadow(true);
	FarISM->SetCastShadow(false);   // research/05: no shadow cast beyond ~80 m
}

FTransform ADhCrowdInstanceRenderer::ParkedTransform() const
{
	return FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, -500000.f), FVector(0.001f));
}

void ADhCrowdInstanceRenderer::Setup(UStaticMesh* Mesh, UMaterialInterface* Material, int32 NearCapacity, int32 FarCapacity,
	bool bPivotAtFeet, float ProxyHeightCm)
{
	bPivotFeet = bPivotAtFeet;
	ProxyHeight = ProxyHeightCm;

	auto Build = [&](UInstancedStaticMeshComponent* C, int32 Capacity, TArray<int32>& Free)
	{
		C->SetStaticMesh(Mesh);
		if (Material) { C->SetMaterial(0, Material); }
		C->ClearInstances();
		Free.Reset();
		for (int32 i = 0; i < Capacity; ++i)
		{
			const int32 Idx = C->AddInstance(ParkedTransform(), /*bWorldSpace*/ true);
			Free.Add(Idx);
		}
		// Hand out low indices first.
		Algo::Reverse(Free);
		C->MarkRenderStateDirty();
	};
	Build(NearISM, NearCapacity, FreeNear);
	Build(FarISM, FarCapacity, FreeFar);
	FarISM->SetCullDistances(0, 18000); // beyond 180 m nothing is drawn
	NearUsed = FarUsed = 0;
}

int32 ADhCrowdInstanceRenderer::AllocSlot(bool bFar)
{
	TArray<int32>& Free = bFar ? FreeFar : FreeNear;
	if (Free.Num() == 0) { return INDEX_NONE; }
	(bFar ? FarUsed : NearUsed)++;
	return Free.Pop(EAllowShrinking::No);
}

void ADhCrowdInstanceRenderer::FreeSlot(bool bFar, int32 Slot)
{
	if (Slot == INDEX_NONE) { return; }
	UInstancedStaticMeshComponent* C = Comp(bFar);
	C->UpdateInstanceTransform(Slot, ParkedTransform(), /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
	(bFar ? bDirtyFar : bDirtyNear) = true;
	(bFar ? FreeFar : FreeNear).Add(Slot);
	(bFar ? FarUsed : NearUsed)--;
}

void ADhCrowdInstanceRenderer::SetSlot(bool bFar, int32 Slot, const FVector& FeetPos, float YawDeg, float Scale, float GaitPhase, bool bWalking)
{
	if (Slot == INDEX_NONE) { return; }
	// Placeholder gait: a small vertical bob and sway so a crowd reads as walking until VAT meshes exist.
	const float Bob = bWalking ? FMath::Abs(FMath::Sin(GaitPhase)) * 3.f * Scale : 0.f;
	const float Roll = bWalking ? FMath::Sin(GaitPhase) * 2.5f : 0.f;

	// UE's unit cylinder is 100 x 100 x 100 with the pivot in the centre; scale it to a 45 cm wide person.
	FVector S(Scale);
	FVector Loc = FeetPos + FVector(0.f, 0.f, Bob);
	if (!bPivotFeet)
	{
		S = FVector(0.45f * Scale, 0.45f * Scale, (ProxyHeight / 100.f) * Scale);
		Loc.Z += 0.5f * ProxyHeight * Scale;
	}
	const FTransform T(FRotator(0.f, YawDeg, Roll), Loc, S);
	Comp(bFar)->UpdateInstanceTransform(Slot, T, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
	(bFar ? bDirtyFar : bDirtyNear) = true;
}

void ADhCrowdInstanceRenderer::SetSlotColor(bool bFar, int32 Slot, const FLinearColor& Tint, float Skin)
{
	if (Slot == INDEX_NONE) { return; }
	UInstancedStaticMeshComponent* C = Comp(bFar);
	C->SetCustomDataValue(Slot, 0, Tint.R, false);
	C->SetCustomDataValue(Slot, 1, Tint.G, false);
	C->SetCustomDataValue(Slot, 2, Tint.B, false);
	C->SetCustomDataValue(Slot, 3, Skin, false);
	(bFar ? bDirtyFar : bDirtyNear) = true;
}

void ADhCrowdInstanceRenderer::Flush()
{
	if (bDirtyNear) { NearISM->MarkRenderStateDirty(); bDirtyNear = false; }
	if (bDirtyFar) { FarISM->MarkRenderStateDirty(); bDirtyFar = false; }
}
