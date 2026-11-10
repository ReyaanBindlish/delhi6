#include "Crowd/DhCrowdCharacter.h"

#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ADhCrowdCharacter::ADhCrowdCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	bReplicates = false;
	AutoPossessAI = EAutoPossessAI::Disabled;
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(34.f, 90.f);
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	SkelMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	SkelMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SkelMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	SkelMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bRunPhysicsWithNoController = true;      // no AIController: the subsystem drives input directly
	Move->bOrientRotationToMovement = false;
	Move->bUseControllerDesiredRotation = false;
	Move->MaxWalkSpeed = 140.f;
	Move->MaxAcceleration = 800.f;
	Move->BrakingDecelerationWalking = 900.f;
	Move->bUseRVOAvoidance = true;
	Move->AvoidanceConsiderationRadius = 260.f;
	Move->AvoidanceWeight = 0.5f;
	Move->NavAgentProps.bCanCrouch = false;
}

void ADhCrowdCharacter::Acquire(const FDhCrowdAgent& Agent, const FDhCrowdLoadedVisual& Visual,
	UMaterialInterface* MaterialOverride, FName TintParam, FName SkinParam, bool bHero)
{
	bInUse = true;
	USkeletalMeshComponent* SkelMesh = GetMesh();

	WalkAnim = Visual.Walk;
	IdleAnim = Visual.Idle;
	CurrentAnim = nullptr;

	if (SkelMesh->GetSkeletalMeshAsset() != Visual.Mesh)
	{
		SkelMesh->SetSkeletalMeshAsset(Visual.Mesh);
		Mids.Reset();
	}

	// One dynamic instance per material slot, created once per mesh change and re-tinted per agent.
	if (Mids.Num() == 0)
	{
		const int32 NumMats = SkelMesh->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			if (MaterialOverride) { SkelMesh->SetMaterial(i, MaterialOverride); }
			UMaterialInstanceDynamic* Mid = SkelMesh->CreateDynamicMaterialInstance(i, SkelMesh->GetMaterial(i));
			Mids.Add(Mid);
		}
	}
	for (UMaterialInstanceDynamic* Mid : Mids)
	{
		if (!Mid) { continue; }
		Mid->SetVectorParameterValue(TintParam, Agent.Tint);
		Mid->SetScalarParameterValue(SkinParam, Agent.Skin);
	}

	SetActorScale3D(FVector(Agent.Scale));
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(false);
	GetCharacterMovement()->SetComponentTickEnabled(true);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SkelMesh->SetComponentTickEnabled(true);
	SetHero(bHero);
}

void ADhCrowdCharacter::Release()
{
	bInUse = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetComponentTickEnabled(false);
	GetMesh()->SetComponentTickEnabled(false);
	GetMesh()->Stop();
	CurrentAnim = nullptr;
}

void ADhCrowdCharacter::SetHero(bool bHero)
{
	bHeroNow = bHero;
	// Skeletal tier (LOD1): 30 Hz pose update, no RVO. Hero tier (LOD0): full rate and avoidance.
	GetMesh()->SetComponentTickInterval(bHero ? 0.f : (1.f / 30.f));
	GetCharacterMovement()->bUseRVOAvoidance = bHero;
}

float ADhCrowdCharacter::GetFeetZ() const
{
	return static_cast<float>(GetActorLocation().Z) - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

void ADhCrowdCharacter::PlayLoop(UAnimSequence* Anim, float PlayRate)
{
	if (!Anim) { return; }
	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (CurrentAnim != Anim)
	{
		CurrentAnim = Anim;
		SkelMesh->PlayAnimation(Anim, true);
	}
	SkelMesh->SetPlayRate(PlayRate);
}

void ADhCrowdCharacter::Drive(const FVector& TargetPos, const FVector& TravelDir, float SpeedCms, EDhCrowdActivity Activity, float Dt)
{
	if (!bInUse) { return; }

	const FVector Loc = GetActorLocation();
	FVector ToTarget(TargetPos.X - Loc.X, TargetPos.Y - Loc.Y, 0.f);
	const float Dist = static_cast<float>(ToTarget.Size());

	if (Dist > TeleportDistanceCm)
	{
		// Fell behind (lag, crowd squeeze, just promoted): snap onto the lane, keep our own Z.
		TeleportTo(FVector(TargetPos.X, TargetPos.Y, Loc.Z), GetActorRotation(), false, true);
		return;
	}

	UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bStanding = (Activity == EDhCrowdActivity::Idle || Activity == EDhCrowdActivity::Work) || Dist < 12.f;
	if (!bStanding)
	{
		// Chase the virtual point slightly faster than it moves so the gap closes.
		Move->MaxWalkSpeed = SpeedCms * 1.15f;
		AddMovementInput(ToTarget / FMath::Max(Dist, 1.f), 1.f);
	}

	// Face travel direction (or the way the agent was facing when it stopped).
	if (!TravelDir.IsNearlyZero())
	{
		const FRotator Want(0.f, static_cast<float>(TravelDir.Rotation().Yaw), 0.f);
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Want, Dt, bStanding ? 90.f : 240.f));
	}

	const float Vel = static_cast<float>(Move->Velocity.Size2D());
	if (Vel > 20.f)
	{
		PlayLoop(WalkAnim ? WalkAnim.Get() : IdleAnim.Get(), FMath::Clamp(Vel / 140.f, 0.5f, 2.6f));
	}
	else
	{
		PlayLoop(IdleAnim ? IdleAnim.Get() : WalkAnim.Get(), 1.f);
	}
}
