#include "Physics/FallTrackerComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DayaVitalsComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UFallTrackerComponent::UFallTrackerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UFallTrackerComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	if (Character.IsValid())
	{
		Mesh = Character->GetMesh();
		Character->LandedDelegate.AddDynamic(this, &UFallTrackerComponent::OnLanded);
		LastZ = GetCurrentHeightCm();
	}
}

float UFallTrackerComponent::GetCurrentHeightCm() const
{
	const ACharacter* C = Character.Get();
	if (!C)
	{
		return 0.f;
	}
	if (Mesh.IsValid() && Mesh->IsSimulatingPhysics())
	{
		return Mesh->GetBoneLocation(RagdollRootBone).Z;
	}
	const UCapsuleComponent* Cap = C->GetCapsuleComponent();
	return Cap ? Cap->GetComponentLocation().Z - Cap->GetScaledCapsuleHalfHeight() : C->GetActorLocation().Z;
}

void UFallTrackerComponent::BeginTracking()
{
	bTracking = true;
	bRagFalling = false;
	PeakZ = GetCurrentHeightCm();
	LastZ = PeakZ;
}

EFallSeverity UFallTrackerComponent::Classify(float HeightMeters) const
{
	if (HeightMeters < MinReportMeters) return EFallSeverity::None;
	if (HeightMeters >= LethalMeters)   return EFallSeverity::Lethal;
	if (HeightMeters >= InjuryMeters)   return EFallSeverity::Injury;
	return EFallSeverity::Minor;
}

void UFallTrackerComponent::ReportLanding(float HeightMeters)
{
	bTracking = false;
	bRagFalling = false;
	const EFallSeverity Sev = Classify(HeightMeters);
	if (Sev == EFallSeverity::None)
	{
		return;
	}
	if (bReportToVitals)
	{
		if (UDayaVitalsComponent* Vitals = GetOwner()->FindComponentByClass<UDayaVitalsComponent>())
		{
			Vitals->ApplyFall(HeightMeters);
		}
	}
	OnFallCompleted.Broadcast(HeightMeters, Sev);
}

void UFallTrackerComponent::OnLanded(const FHitResult& Hit)
{
	if (!bTracking)
	{
		return;
	}
	const float DropM = FMath::Max(0.f, PeakZ - GetCurrentHeightCm()) / 100.f;
	ReportLanding(DropM);
}

void UFallTrackerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACharacter* C = Character.Get();
	if (!C || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Z = GetCurrentHeightCm();
	const bool bRagdoll = Mesh.IsValid() && Mesh->IsSimulatingPhysics();

	if (bRagdoll)
	{
		const float Vz = (Z - LastZ) / DeltaTime;
		if (!bRagFalling)
		{
			PeakZ = FMath::Max(PeakZ, Z);
			if (Vz < -300.f) { bRagFalling = true; bTracking = true; }
			else if (!bTracking) { PeakZ = Z; }
		}
		else
		{
			PeakZ = FMath::Max(PeakZ, Z);
			if (Vz > -80.f)
			{
				ReportLanding(FMath::Max(0.f, PeakZ - Z) / 100.f);
			}
		}
	}
	else if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
	{
		if (Move->IsFalling())
		{
			if (!bTracking) { bTracking = true; PeakZ = Z; }
			PeakZ = FMath::Max(PeakZ, Z);
		}
		// Landing is delivered by LandedDelegate.
	}
	LastZ = Z;
}
