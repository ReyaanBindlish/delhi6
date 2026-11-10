#include "Characters/CastTellComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogCastTell, Log, All);

UCastTellComponent::UCastTellComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Tells.Add(TEXT("ThumbCrack"), FCastTell());
}

bool UCastTellComponent::IsOnCooldown(FName TellId) const
{
	const FCastTell* Tell = Tells.Find(TellId);
	const double* Last = LastPlayed.Find(TellId);
	if (!Tell || !Last || !GetWorld())
	{
		return false;
	}
	return (GetWorld()->GetTimeSeconds() - *Last) < Tell->CooldownSeconds;
}

int32 UCastTellComponent::TriggerTell(FName TellId, const TArray<AActor*>& Observers)
{
	const FCastTell* Tell = Tells.Find(TellId);
	AActor* Owner = GetOwner();
	if (!Tell || !Owner || !GetWorld())
	{
		UE_LOG(LogCastTell, Warning, TEXT("TriggerTell: unknown tell '%s'."), *TellId.ToString());
		return 0;
	}
	if (IsOnCooldown(TellId))
	{
		return 0;
	}
	LastPlayed.Add(TellId, GetWorld()->GetTimeSeconds());

	// The visible part: the montage on the owner's mesh.
	if (!Tell->Montage.IsNull())
	{
		USkeletalMeshComponent* Mesh = nullptr;
		if (const ACharacter* Char = Cast<ACharacter>(Owner))
		{
			Mesh = Char->GetMesh();
		}
		if (!Mesh)
		{
			Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
		}
		if (Mesh)
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (UAnimMontage* Montage = Tell->Montage.LoadSynchronous())
				{
					AnimInstance->Montage_Play(Montage);
				}
			}
		}
	}

	// The consequence: observers who are near enough and facing the owner.
	int32 Noticed = 0;
	const float CosLimit = FMath::Cos(FMath::DegreesToRadians(Tell->ObserveHalfAngleDegrees));
	for (AActor* Observer : Observers)
	{
		if (!Observer || Observer == Owner)
		{
			continue;
		}
		const FVector ToOwner = Owner->GetActorLocation() - Observer->GetActorLocation();
		const float Dist = ToOwner.Size();
		if (Dist > Tell->MaxObserveDistanceCm || Dist < KINDA_SMALL_NUMBER)
		{
			continue;
		}
		const float CosAngle = FVector::DotProduct(Observer->GetActorForwardVector(), ToOwner / Dist);
		if (CosAngle < CosLimit)
		{
			continue;
		}
		if (USuspicionComponent* Suspicion = Observer->FindComponentByClass<USuspicionComponent>())
		{
			Suspicion->AddSuspicion(Tell->Cause, Tell->SuspicionScale);
			++Noticed;
		}
	}

	OnTell.Broadcast(TellId, Noticed);
	return Noticed;
}

int32 UCastTellComponent::CrackThumb(const TArray<AActor*>& Observers)
{
	return TriggerTell(TEXT("ThumbCrack"), Observers);
}
