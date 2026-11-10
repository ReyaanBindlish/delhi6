#include "Physics/RailingComponent.h"

#include "Components/CapsuleComponent.h"
#include "DayaVitalsComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Physics/FallTrackerComponent.h"
#include "Physics/HitReactionComponent.h"
#include "TimerManager.h"

URailingComponent::URailingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InitBoxExtent(FVector(150.f, 5.f, 45.f));
	SetCanEverAffectNavigation(false);
}

float URailingComponent::GetRailHeightCm() const
{
	switch (Standard)
	{
	case ERailStandard::Builder90: return 90.f;
	case ERailStandard::Code110:   return 110.f;
	default:                       return FMath::Max(10.f, CustomRailHeightCm);
	}
}

FVector URailingComponent::GetDropDirection() const
{
	const FVector Right = GetRightVector();
	return bDropSideIsNegativeY ? -Right : Right;
}

void URailingComponent::OnRegister()
{
	Super::OnRegister();
	SetBoxExtent(FVector(RailLengthCm * 0.5f, RailThicknessCm * 0.5f, GetRailHeightCm() * 0.5f), false);

	// The rail blocks bodies and vehicles only; bullets and camera traces pass (a rail is open metalwork).
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	SetHiddenInGame(true);
}

bool URailingComponent::EvaluateOverRail(float ComHeightM, float RailHeightM, float ComDistanceM, float PushSpeedMS,
                                         float PushEfficiency, float GripFactor, FRailEvaluation& OutEval)
{
	constexpr float G = 9.81f;
	const float Lever = ComHeightM - RailHeightM;
	const float D = FMath::Max(ComDistanceM, 0.05f);
	const float R = FMath::Sqrt(D * D + Lever * Lever);
	const float EnergyPerKg = G * (R - Lever) * FMath::Max(GripFactor, 1.f);
	const float Eff = FMath::Clamp(PushEfficiency, 0.05f, 1.f);

	OutEval.ComHeightM = ComHeightM;
	OutEval.RailHeightM = RailHeightM;
	OutEval.LeverM = Lever;
	OutEval.ComDistanceM = D;
	OutEval.RequiredSpeedMS = FMath::Sqrt(2.f * EnergyPerKg / Eff);
	OutEval.EffectivePushSpeedMS = FMath::Max(PushSpeedMS, 0.f);
	OutEval.bGoesOver = OutEval.EffectivePushSpeedMS >= OutEval.RequiredSpeedMS;
	return OutEval.bGoesOver;
}

FRailEvaluation URailingComponent::EvaluateCharacter(ACharacter* Victim, FVector PushVelocityCmS) const
{
	FRailEvaluation Eval;
	if (!Victim || !Victim->GetCapsuleComponent())
	{
		return Eval;
	}
	const UCapsuleComponent* Capsule = Victim->GetCapsuleComponent();
	const float RailH = GetRailHeightCm();
	const FVector Origin = GetComponentLocation();
	const FVector Drop = GetDropDirection();

	const FVector Rel = Capsule->GetComponentLocation() - Origin;
	const float SideOffset = FVector::DotProduct(Rel, GetForwardVector());   // along the rail
	const float BehindCm = -FVector::DotProduct(Rel, Drop);                   // positive = on the safe side
	const float R = Capsule->GetScaledCapsuleRadius();
	Eval.bNearRail = FMath::Abs(SideOffset) <= RailLengthCm * 0.5f + R && BehindCm > -R && BehindCm < R + 80.f;
	if (!Eval.bNearRail)
	{
		return Eval;
	}

	const float FloorZ = Origin.Z - RailH * 0.5f;
	const float FeetZ = Capsule->GetComponentLocation().Z - Capsule->GetScaledCapsuleHalfHeight();
	const float StandingHeightCm = Capsule->GetScaledCapsuleHalfHeight() * 2.f;
	const float ComHeightCm = (FeetZ - FloorZ) + StandingHeightCm * ComHeightFraction;

	const float ComDistCm = BehindCm - RailThicknessCm * 0.5f - R + BodyHalfDepthCm;

	// Push component toward the drop side, plus the victim's own velocity that way.
	const float PushCmS = FVector::DotProduct(PushVelocityCmS + Victim->GetVelocity(), Drop);

	bool bCanGrab = true;
	if (const UDayaVitalsComponent* Vitals = Victim->FindComponentByClass<UDayaVitalsComponent>())
	{
		bCanGrab = !Vitals->IsInputLocked();
	}
	const float Grip = bCanGrab ? GripRecoveryFactor : 1.f;

	const bool bNear = Eval.bNearRail;
	EvaluateOverRail(ComHeightCm / 100.f, RailH / 100.f, ComDistCm / 100.f, PushCmS / 100.f, PushEfficiency, Grip, Eval);
	Eval.bNearRail = bNear;
	return Eval;
}

bool URailingComponent::TryShove(ACharacter* Victim, FVector ShoveVelocityCmS, FRailEvaluation& OutEval)
{
	OutEval = EvaluateCharacter(Victim, ShoveVelocityCmS);
	if (!Victim || !OutEval.bNearRail)
	{
		return false;
	}

	if (!OutEval.bGoesOver)
	{
		// The rail catches him: stagger back, no fall.
		Victim->LaunchCharacter(-GetDropDirection() * 120.f, true, false);
		return false;
	}

	// Make sure falls are measured.
	UFallTrackerComponent* Tracker = Victim->FindComponentByClass<UFallTrackerComponent>();
	if (!Tracker)
	{
		Tracker = NewObject<UFallTrackerComponent>(Victim, TEXT("FallTracker"));
		Tracker->RegisterComponent();
	}
	Tracker->BeginTracking();

	// Let the capsule through the rail (only for this victim, only for a moment).
	UCapsuleComponent* Capsule = Victim->GetCapsuleComponent();
	if (Capsule)
	{
		Capsule->IgnoreComponentWhenMoving(this, true);
		TWeakObjectPtr<ACharacter> WeakVictim(Victim);
		FTimerHandle Handle;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(Handle,
				FTimerDelegate::CreateWeakLambda(this, [this, WeakVictim]()
				{
					if (WeakVictim.IsValid() && WeakVictim->GetCapsuleComponent())
					{
						WeakVictim->GetCapsuleComponent()->IgnoreComponentWhenMoving(this, false);
					}
				}), PassThroughSeconds, false);
		}
	}

	// Lift slightly so the COM actually clears the rail top.
	const FVector Launch = ShoveVelocityCmS + FVector(0.f, 0.f, 60.f);
	Victim->LaunchCharacter(Launch, true, true);

	if (bRagdollOnFall)
	{
		if (UHitReactionComponent* HR = Victim->FindComponentByClass<UHitReactionComponent>())
		{
			HR->EnterFullRagdoll(Launch);
		}
	}
	return true;
}
