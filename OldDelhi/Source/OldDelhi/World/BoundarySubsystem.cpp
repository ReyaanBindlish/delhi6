#include "World/BoundarySubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Components/PrimitiveComponent.h"
#include "UObject/SoftObjectPath.h"
#include "World/WorldBoundary.h"

bool UBoundarySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UBoundarySubsystem::RegisterBoundary(AWorldBoundary* Boundary)
{
	if (Boundary) { Boundaries.AddUnique(Boundary); }
}

void UBoundarySubsystem::UnregisterBoundary(AWorldBoundary* Boundary)
{
	Boundaries.Remove(Boundary);
}

void UBoundarySubsystem::Tick(float DeltaTime)
{
	Accum += DeltaTime;
	const float Step = 1.f / FMath::Max(1.f, UpdateHz);
	if (Accum < Step) { return; }
	const float Dt = Accum;
	Accum = 0.f;
	Update(Dt);
}

void UBoundarySubsystem::Update(float Dt)
{
	UWorld* World = GetWorld();
	if (!World) { return; }
	Boundaries.RemoveAll([](const TWeakObjectPtr<AWorldBoundary>& B) { return !B.IsValid(); });
	if (Boundaries.IsEmpty()) { return; }

	if (!BoundaryCollection && !bCollectionSearched)
	{
		bCollectionSearched = true;
		BoundaryCollection = Cast<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Delhi/Materials/MPC_Boundary.MPC_Boundary")).TryLoad());
	}
	UMaterialParameterCollectionInstance* MPC = BoundaryCollection ? World->GetParameterCollectionInstance(BoundaryCollection) : nullptr;

	bool bFirstPlayer = true;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn) { continue; }
		const FVector Loc = Pawn->GetActorLocation();

		// nearest boundary
		AWorldBoundary* Best = nullptr;
		float BestDist = TNumericLimits<float>::Max();
		FVector Closest = Loc;
		FVector2D Inward = FVector2D::ZeroVector;
		for (const TWeakObjectPtr<AWorldBoundary>& WB : Boundaries)
		{
			AWorldBoundary* B = WB.Get();
			if (!B || !B->HasPolygon()) { continue; }
			FVector C; FVector2D In;
			const float D = B->QueryDistance(Loc, C, In);
			if (FMath::Abs(D) < FMath::Abs(BestDist)) { Best = B; BestDist = D; Closest = C; Inward = In; }
		}
		if (!Best) { continue; }

		const float Reveal = RevealRadiusCm;
		const float Proximity = FMath::Clamp(1.f - FMath::Max(BestDist, 0.f) / FMath::Max(1.f, Reveal), 0.f, 1.f);
		if (bFirstPlayer)
		{
			bFirstPlayer = false;
			LastProximity = Proximity;
			if (MPC)
			{
				MPC->SetVectorParameterValue(TEXT("PlayerPos"), FLinearColor((float)Loc.X, (float)Loc.Y, (float)Loc.Z, 1.f));
				MPC->SetScalarParameterValue(TEXT("Proximity"), Proximity);
				MPC->SetScalarParameterValue(TEXT("RevealRadius"), Reveal);
			}
			OnBoundaryProximity.Broadcast(Proximity, BestDist);
		}

		// feedback: hum + post-process
		const float Hum = FMath::Clamp(1.f - FMath::Max(BestDist, 0.f) / FMath::Max(1.f, Best->HumRangeCm), 0.f, 1.f);
		const float Post = FMath::Clamp(1.f - FMath::Max(BestDist, 0.f) / FMath::Max(1.f, Best->PostEffectRangeCm), 0.f, 1.f);
		Best->UpdateFeedback(Hum, Closest, Post);

		// gentle push back
		const FVector Inward3(Inward.X, Inward.Y, 0.f);
		if (bEnablePushBack && BestDist < Best->PushStartDistanceCm && !Inward.IsNearlyZero())
		{
			const float Alpha = BestDist <= 0.f ? 1.f : 1.f - BestDist / FMath::Max(1.f, Best->PushStartDistanceCm);
			const FVector Vel = Pawn->GetVelocity();
			const float Approach = FVector::DotProduct(Vel, -Inward3); // cm/s towards the wall
			const bool bMovingIn = Approach > 10.f;
			if (bMovingIn || BestDist < Best->PushStartDistanceCm * 0.25f)
			{
				if (const ACharacter* Ch = Cast<ACharacter>(Pawn))
				{
					if (UCharacterMovementComponent* Move = Ch->GetCharacterMovement())
					{
						Move->AddImpulse(Inward3 * Best->PushAccelCmS2 * Alpha * Dt, true);
					}
				}
				else if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
				{
					if (Prim->IsSimulatingPhysics())
					{
						const float SpeedScale = 1.f + Vel.Size() / 1000.f;
						Prim->AddImpulse(Inward3 * Prim->GetMass() * Best->PushAccelCmS2 * Alpha * Dt * SpeedScale, NAME_None, false);
					}
				}
			}
		}

		// glitch sting + rumble, once per approach
		TWeakObjectPtr<AActor> Key(Pawn);
		if (BestDist < Best->TouchDistanceCm)
		{
			if (!TouchedPawns.Contains(Key))
			{
				TouchedPawns.Add(Key);
				OnBoundaryTouched.Broadcast(Pawn, Closest);
				if (bEnableRumble && PC) { PC->PlayDynamicForceFeedback(0.6f, 0.25f, true, true, true, true); }
			}
		}
		else if (BestDist > Best->TouchDistanceCm * 3.f)
		{
			TouchedPawns.Remove(Key);
		}
	}
}
