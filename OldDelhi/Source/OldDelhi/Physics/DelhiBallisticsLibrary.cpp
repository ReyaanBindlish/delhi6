#include "Physics/DelhiBallisticsLibrary.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Physics/DelhiSurfaceLibrary.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"

namespace
{
	const FName BallisticsIgnoreTag(TEXT("BallisticsIgnore"));
	constexpr float PawnThicknessCm = 25.f;
	constexpr float RicochetEnergyKeep = 0.4f;
	constexpr float RicochetScatterDeg = 5.f;

	float EstimateThicknessCm(const FHitResult& Hit, const FVector& Dir, float MaxThicknessCm)
	{
		if (Hit.GetActor() && Hit.GetActor()->IsA<APawn>())
		{
			return PawnThicknessCm;
		}
		UPrimitiveComponent* Comp = Hit.GetComponent();
		if (!Comp)
		{
			return MaxThicknessCm;
		}
		// Reverse trace against the same component: the first thing it hits is the exit face.
		const FVector Far = Hit.ImpactPoint + Dir * MaxThicknessCm;
		const FVector Near = Hit.ImpactPoint - Dir * 0.1f;
		FHitResult Back;
		FCollisionQueryParams P(SCENE_QUERY_STAT(DelhiBallisticsThickness), /*bTraceComplex*/ true);
		if (Comp->LineTraceComponent(Back, Far, Near, P))
		{
			return FMath::Max(0.1f, FVector::Dist(Back.ImpactPoint, Hit.ImpactPoint));
		}
		return MaxThicknessCm; // no exit face within range: treat as a thick solid
	}
}

FDelhiBallisticProfile UDelhiBallisticsLibrary::GetProfile(EDelhiWeaponClass WeaponClass)
{
	FDelhiBallisticProfile P;
	P.WeaponClass = WeaponClass;
	switch (WeaponClass)
	{
	case EDelhiWeaponClass::Pistol9mm:
		P.MuzzleEnergyJ = 500.f; P.MuzzleVelocityCmS = 35000.f; P.SpreadHalfAngleDeg = 0.6f;
		P.MisfireOneIn = 0; P.MaxRangeCm = 25000.f;
		break;
	case EDelhiWeaponClass::Katta:
		// Country-made single shot: weak, inaccurate, unreliable (bible: misfires 1 in 8).
		P.MuzzleEnergyJ = 250.f; P.MuzzleVelocityCmS = 20000.f; P.SpreadHalfAngleDeg = 2.5f;
		P.MisfireOneIn = 8; P.MaxRangeCm = 8000.f; P.MaxImpacts = 3;
		break;
	case EDelhiWeaponClass::Rifle762:
		P.MuzzleEnergyJ = 2500.f; P.MuzzleVelocityCmS = 71000.f; P.SpreadHalfAngleDeg = 0.3f;
		P.MisfireOneIn = 0; P.MaxRangeCm = 80000.f; P.SegmentLengthCm = 2000.f;
		break;
	}
	return P;
}

bool UDelhiBallisticsLibrary::RollMisfire(const FDelhiBallisticProfile& Profile, FRandomStream& Stream)
{
	if (Profile.MisfireOneIn <= 0)
	{
		return false;
	}
	return Stream.RandRange(1, Profile.MisfireOneIn) == 1;
}

bool UDelhiBallisticsLibrary::WouldStop(const FDelhiBallisticProfile& Profile, EDelhiSurface Surface, float ThicknessCm)
{
	return UDelhiSurfaceLibrary::Lookup(Surface).PenetrationJPerCm * ThicknessCm >= Profile.MuzzleEnergyJ;
}

float UDelhiBallisticsLibrary::GetDropAtRangeCm(const UObject* WorldContextObject, const FDelhiBallisticProfile& Profile, float RangeCm)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const float G = World ? FMath::Abs(World->GetGravityZ()) : 980.f;
	const float T = RangeCm / FMath::Max(Profile.MuzzleVelocityCmS, 1.f);
	return 0.5f * G * T * T;
}

bool UDelhiBallisticsLibrary::FireShot(const UObject* WorldContextObject, AActor* Instigator, FVector Origin, FVector AimDirection,
                                       const FDelhiBallisticProfile& Profile, FRandomStream& Stream,
                                       FDelhiShotResult& OutResult, bool bDrawDebug)
{
	OutResult = FDelhiShotResult();
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		return false;
	}
	OutResult.EndPoint = Origin;

	if (RollMisfire(Profile, Stream))
	{
		OutResult.bMisfired = true;
		return false;
	}

	// NO AIM ASSIST: direction is the caller's, plus the weapon's own random spread. Nothing else touches it.
	FVector Dir = AimDirection.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return false;
	}
	if (Profile.SpreadHalfAngleDeg > 0.f)
	{
		Dir = Stream.VRandCone(Dir, FMath::DegreesToRadians(Profile.SpreadHalfAngleDeg));
	}

	const float MuzzleSpeed = FMath::Max(Profile.MuzzleVelocityCmS, 1.f);
	const float MuzzleEnergy = FMath::Max(Profile.MuzzleEnergyJ, 1.f);
	const float Gz = World->GetGravityZ(); // cm/s^2, negative

	FVector Pos = Origin;
	float Speed = MuzzleSpeed;
	float Energy = MuzzleEnergy;
	float Travelled = 0.f;
	int32 Guard = 0;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DelhiBallistics), /*bTraceComplex*/ false, Instigator);
	Params.bReturnPhysicalMaterial = true;

	while (Travelled < Profile.MaxRangeCm && Energy >= Profile.MinEnergyJ && Guard++ < 1024 &&
	       OutResult.Impacts.Num() < Profile.MaxImpacts)
	{
		const float Dt = Profile.SegmentLengthCm / FMath::Max(Speed, 1.f);
		const FVector Vel = Dir * Speed;
		const FVector Next = Pos + Vel * Dt + FVector(0.f, 0.f, 0.5f * Gz * Dt * Dt);
		const FVector NewVel = Vel + FVector(0.f, 0.f, Gz * Dt);

		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Pos, Next, Profile.TraceChannel, Params);

		if (bHit && Hit.GetActor() && Hit.GetActor()->ActorHasTag(BallisticsIgnoreTag))
		{
			Params.AddIgnoredActor(Hit.GetActor());
			continue; // retry the same segment without it
		}

		if (!bHit)
		{
			if (bDrawDebug) { DrawDebugLine(World, Pos, Next, FColor::Yellow, false, 3.f, 0, 0.5f); }
			Travelled += FVector::Dist(Pos, Next);
			Pos = Next;
			Dir = NewVel.GetSafeNormal();
			OutResult.EndPoint = Pos;
			continue;
		}

		if (bDrawDebug) { DrawDebugLine(World, Pos, Hit.ImpactPoint, FColor::Red, false, 3.f, 0, 0.5f); }
		Travelled += FVector::Dist(Pos, Hit.ImpactPoint);
		Dir = (Hit.ImpactPoint - Pos).GetSafeNormal();

		FDelhiBulletImpact Impact;
		Impact.Hit = Hit;
		Impact.Surface = UDelhiSurfaceLibrary::GetSurfaceFromHit(Hit);
		Impact.EnergyBeforeJ = Energy;
		const FDelhiSurfaceData& Data = UDelhiSurfaceLibrary::Lookup(Impact.Surface);
		OutResult.EndPoint = Hit.ImpactPoint;

		// Ricochet: shallow angle on a hard surface.
		const float CosIn = FMath::Clamp(FVector::DotProduct(-Dir, Hit.ImpactNormal), -1.f, 1.f);
		const float AngleFromSurfaceDeg = 90.f - FMath::RadiansToDegrees(FMath::Acos(CosIn));
		if (Data.RicochetAngleDeg > 0.f && AngleFromSurfaceDeg < Data.RicochetAngleDeg)
		{
			Dir = Dir - 2.f * FVector::DotProduct(Dir, Hit.ImpactNormal) * Hit.ImpactNormal;
			Dir = Stream.VRandCone(Dir.GetSafeNormal(), FMath::DegreesToRadians(RicochetScatterDeg));
			Energy *= RicochetEnergyKeep;
			Speed = MuzzleSpeed * FMath::Sqrt(Energy / MuzzleEnergy);
			Pos = Hit.ImpactPoint + Hit.ImpactNormal * 0.5f;
			Impact.Result = EDelhiImpactResult::Ricocheted;
			Impact.EnergyAfterJ = Energy;
			OutResult.Impacts.Add(Impact);
			continue;
		}

		// Penetration: energy absorbed = J/cm * thickness crossed.
		const float Thickness = EstimateThicknessCm(Hit, Dir, Profile.MaxThicknessCm);
		const float Loss = Data.PenetrationJPerCm * Thickness;
		Impact.ThicknessCm = Thickness;
		if (Loss >= Energy)
		{
			Impact.Result = EDelhiImpactResult::Stopped;
			Impact.EnergyAfterJ = 0.f;
			OutResult.Impacts.Add(Impact);
			break;
		}
		Energy -= Loss;
		Speed = MuzzleSpeed * FMath::Sqrt(Energy / MuzzleEnergy);
		Impact.Result = EDelhiImpactResult::Penetrated;
		Impact.EnergyAfterJ = Energy;
		OutResult.Impacts.Add(Impact);
		Pos = Hit.ImpactPoint + Dir * (Thickness + 0.5f);
		if (bDrawDebug) { DrawDebugPoint(World, Pos, 6.f, FColor::Green, false, 3.f); }
	}
	return true;
}
