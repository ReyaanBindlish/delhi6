#include "Physics/HitReactionComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"

// ------------------------------------------------------------------------------------------------- budget
void UDelhiRagdollBudgetSubsystem::Register(UHitReactionComponent* Comp)
{
	if (!Comp)
	{
		return;
	}
	Active.RemoveAll([](const TWeakObjectPtr<UHitReactionComponent>& W) { return !W.IsValid() || W->IsFrozen() || !W->IsRagdoll(); });
	Active.AddUnique(Comp);
	while (Active.Num() > FMath::Max(1, MaxActiveRagdolls))
	{
		if (UHitReactionComponent* Oldest = Active[0].Get())
		{
			Oldest->FreezeRagdoll();
		}
		Active.RemoveAt(0);
	}
}

void UDelhiRagdollBudgetSubsystem::Unregister(UHitReactionComponent* Comp)
{
	Active.Remove(Comp);
}

int32 UDelhiRagdollBudgetSubsystem::GetActiveRagdollCount() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<UHitReactionComponent>& W : Active)
	{
		if (W.IsValid() && W->IsRagdoll() && !W->IsFrozen())
		{
			++N;
		}
	}
	return N;
}

// ------------------------------------------------------------------------------------------------- component
UHitReactionComponent::UHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // only while a partial blend is running
}

void UHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacter* C = Cast<ACharacter>(GetOwner()))
	{
		Mesh = C->GetMesh();
		Capsule = C->GetCapsuleComponent();
	}
	else
	{
		Mesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (Mesh.IsValid())
	{
		SavedMeshRelative = Mesh->GetRelativeTransform();
		SavedCollisionProfile = Mesh->GetCollisionProfileName();
		PhysAnim = NewObject<UPhysicalAnimationComponent>(GetOwner(), TEXT("DelhiPhysicalAnimation"));
		PhysAnim->RegisterComponent();
		PhysAnim->SetSkeletalMeshComponent(Mesh.Get());
	}
}

void UHitReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UDelhiRagdollBudgetSubsystem* Budget = World->GetSubsystem<UDelhiRagdollBudgetSubsystem>())
		{
			Budget->Unregister(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UHitReactionComponent::NormalizeMass()
{
	if (bMassNormalized || !Mesh.IsValid())
	{
		return;
	}
	bMassNormalized = true;
	const float Current = Mesh->CalculateMass();
	if (Current > 1.f && FMath::Abs(Current - BodyMassKg) > 0.25f * BodyMassKg)
	{
		const float Scale = BodyMassKg / Current;
		for (FBodyInstance* Body : Mesh->Bodies)
		{
			if (Body)
			{
				Body->SetMassScale(Body->MassScale * Scale);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[HitReaction] %s physics asset mass %.1f kg rescaled to %.1f kg"), *GetOwner()->GetName(), Current, BodyMassKg);
	}
}

void UHitReactionComponent::ApplyHit(FName Bone, FVector HitLocation, FVector ImpulseDirection, float ImpulseNs)
{
	USkeletalMeshComponent* M = Mesh.Get();
	if (!M || !M->GetPhysicsAsset())
	{
		return;
	}
	const FVector Impulse = ImpulseDirection.GetSafeNormal() * ImpulseNs * 100.f; // N*s -> kg*cm/s

	if (bRagdoll)
	{
		if (!bFrozen)
		{
			M->AddImpulseAtLocation(Impulse, HitLocation, Bone);
		}
		return;
	}
	if (ImpulseNs >= FullRagdollThresholdNs)
	{
		EnterFullRagdoll(Impulse / FMath::Max(BodyMassKg, 1.f));
		return;
	}

	// Partial: the bone chain below Bone goes loose, is driven back toward the animation pose, then blends out.
	NormalizeMass();
	if (Bone == NAME_None)
	{
		Bone = M->GetBoneName(0);
	}
	if (PhysAnim)
	{
		FPhysicalAnimationData Data;
		Data.bIsLocalSimulation = true;
		Data.OrientationStrength = 1000.f;
		Data.AngularVelocityStrength = 100.f;
		Data.PositionStrength = 1000.f;
		Data.VelocityStrength = 100.f;
		Data.MaxLinearForce = 0.f;
		Data.MaxAngularForce = 0.f;
		PhysAnim->ApplyPhysicalAnimationSettingsBelow(Bone, Data, true);
	}
	M->SetAllBodiesBelowSimulatePhysics(Bone, true, true);
	M->SetAllBodiesBelowPhysicsBlendWeight(Bone, PartialPeakBlendWeight, false, true);
	M->AddImpulseAtLocation(Impulse, HitLocation, Bone);

	FActiveBlend* Existing = Blends.FindByPredicate([&](const FActiveBlend& B) { return B.Bone == Bone; });
	if (Existing)
	{
		Existing->TimeLeft = Existing->Total = PartialBlendOutSeconds;
	}
	else
	{
		FActiveBlend B; B.Bone = Bone; B.TimeLeft = B.Total = PartialBlendOutSeconds;
		Blends.Add(B);
	}
	SetComponentTickEnabled(true);
}

void UHitReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	USkeletalMeshComponent* M = Mesh.Get();
	if (!M || bRagdoll)
	{
		Blends.Reset();
		SetComponentTickEnabled(false);
		return;
	}
	for (int32 i = Blends.Num() - 1; i >= 0; --i)
	{
		FActiveBlend& B = Blends[i];
		B.TimeLeft -= DeltaTime;
		if (B.TimeLeft <= 0.f)
		{
			M->SetAllBodiesBelowPhysicsBlendWeight(B.Bone, 0.f, false, true);
			M->SetAllBodiesBelowSimulatePhysics(B.Bone, false, true);
			Blends.RemoveAtSwap(i);
		}
		else
		{
			M->SetAllBodiesBelowPhysicsBlendWeight(B.Bone, PartialPeakBlendWeight * (B.TimeLeft / B.Total), false, true);
		}
	}
	if (Blends.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}

void UHitReactionComponent::EnterFullRagdoll(FVector LinearVelocityCmS)
{
	USkeletalMeshComponent* M = Mesh.Get();
	if (bRagdoll || !M || !M->GetPhysicsAsset())
	{
		return;
	}
	NormalizeMass();

	if (UWorld* World = GetWorld())
	{
		if (UDelhiRagdollBudgetSubsystem* Budget = World->GetSubsystem<UDelhiRagdollBudgetSubsystem>())
		{
			bRagdoll = true; // so the budget counts us
			bFrozen = false;
			Budget->Register(this);
		}
	}
	bRagdoll = true;
	bFrozen = false;

	if (UCapsuleComponent* Cap = Capsule.Get())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (ACharacter* C = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
			Move->DisableMovement();
		}
	}
	M->SetCollisionProfileName(RagdollCollisionProfile);
	M->SetAllBodiesSimulatePhysics(true);
	M->SetSimulatePhysics(true);
	M->WakeAllRigidBodies();
	M->bBlendPhysics = true;
	M->SetAllPhysicsLinearVelocity(LinearVelocityCmS);

	OnRagdollStarted.Broadcast();
}

void UHitReactionComponent::FreezeRagdoll()
{
	if (!bRagdoll || bFrozen)
	{
		return;
	}
	bFrozen = true;
	if (USkeletalMeshComponent* M = Mesh.Get())
	{
		M->PutAllRigidBodiesToSleep(); // keeps the pose, costs ~nothing
	}
}

void UHitReactionComponent::RecoverFromRagdoll()
{
	USkeletalMeshComponent* M = Mesh.Get();
	ACharacter* C = Cast<ACharacter>(GetOwner());
	if (!bRagdoll || !M || !C)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (UDelhiRagdollBudgetSubsystem* Budget = World->GetSubsystem<UDelhiRagdollBudgetSubsystem>())
		{
			Budget->Unregister(this);
		}
	}
	const FVector PelvisLoc = M->GetBoneLocation(RagdollRootBone);
	M->SetAllBodiesSimulatePhysics(false);
	M->SetSimulatePhysics(false);
	M->bBlendPhysics = false;
	M->SetCollisionProfileName(SavedCollisionProfile);

	if (UCapsuleComponent* Cap = Capsule.Get())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		C->SetActorLocation(PelvisLoc + FVector(0.f, 0.f, Cap->GetScaledCapsuleHalfHeight()), false, nullptr, ETeleportType::TeleportPhysics);
		M->AttachToComponent(Cap, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		M->SetRelativeTransform(SavedMeshRelative);
	}
	if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Falling);
	}
	bRagdoll = false;
	bFrozen = false;
	OnRagdollEnded.Broadcast();
}
