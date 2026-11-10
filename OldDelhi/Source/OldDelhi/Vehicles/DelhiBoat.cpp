#include "Vehicles/DelhiBoat.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// ------------------------------------------------------------------------------------------------- buoyancy
UBoatBuoyancyComponent::UBoatBuoyancyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	// 8 points under the placeholder hull (bottom at z = -45 cm).
	for (float X : { -300.f, -100.f, 100.f, 300.f })
	{
		BuoyancyPoints.Add(FVector(X, -90.f, -45.f));
		BuoyancyPoints.Add(FVector(X, 90.f, -45.f));
	}
}

float UBoatBuoyancyComponent::GetWaterHeightAt(const FVector& P) const
{
	float H = WaterLevelZCm;
	if (bUseWaves)
	{
		const float K = 2.f * PI / FMath::Max(WaveLengthCm, 1.f);
		const float W = 2.f * PI / FMath::Max(WavePeriodSeconds, 0.1f);
		H += WaveAmplitudeCm * FMath::Sin(K * P.X - W * TimeSeconds);
		H += 0.4f * WaveAmplitudeCm * FMath::Sin(K * 1.7f * P.Y + W * 1.3f * TimeSeconds + 1.1f);
	}
	return H;
}

void UBoatBuoyancyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TimeSeconds += DeltaTime;

	UPrimitiveComponent* Prim = GetOwner() ? Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()) : nullptr;
	if (!Prim || !Prim->IsSimulatingPhysics() || BuoyancyPoints.Num() == 0)
	{
		return;
	}
	const float Mass = Prim->GetMass(); // kg
	const float N = static_cast<float>(BuoyancyPoints.Num());
	// Each point can carry 2 * weight/N at full submersion, so average 50% submerged = floating at design draft.
	const float MaxPointForce = 2.f * Mass * 980.f / N; // kg*cm/s^2 (UE force units)
	const float DampPerPoint = HeaveDamping * Mass / N;

	float SubmergedSum = 0.f;
	const FTransform& T = Prim->GetComponentTransform();
	for (const FVector& Local : BuoyancyPoints)
	{
		const FVector P = T.TransformPosition(Local);
		const float Depth = GetWaterHeightAt(P) - P.Z;
		const float F = FMath::Clamp(Depth / FMath::Max(SubmersionRangeCm, 1.f), 0.f, 1.f);
		SubmergedSum += F;
		if (F <= 0.f)
		{
			continue;
		}
		const FVector V = Prim->GetPhysicsLinearVelocityAtPoint(P);
		const float Vz = V.Z - GetWaterVelocityCmS().Z;
		const FVector Force(0.f, 0.f, MaxPointForce * F - DampPerPoint * Vz * F);
		Prim->AddForceAtLocation(Force, P);
	}
	LastSubmerged = SubmergedSum / N;
}

// ------------------------------------------------------------------------------------------------- math
float UDelhiBoatMath::EstimateReturnSeconds(float DistanceM, float BoatSpeedKmh, float CurrentKmhAlongReturn, float TurnSeconds)
{
	const float Net = BoatSpeedKmh + CurrentKmhAlongReturn; // + when the current helps the return leg
	if (Net <= 0.1f)
	{
		return -1.f;
	}
	return TurnSeconds + DistanceM / (Net / 3.6f);
}

// ------------------------------------------------------------------------------------------------- boat
ADelhiFishingBoat::ADelhiFishingBoat()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	SetRootComponent(Hull);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		Hull->SetStaticMesh(CubeAsset.Object);
	}
	Hull->SetRelativeScale3D(FVector(8.f, 2.2f, 0.9f)); // 800 x 220 x 90 cm placeholder
	Hull->SetCollisionProfileName(TEXT("PhysicsActor"));
	Hull->SetSimulatePhysics(true);
	Hull->SetEnableGravity(true);
	Hull->SetLinearDamping(0.05f);
	Hull->SetAngularDamping(1.5f);
	Hull->BodyInstance.bOverrideMass = true;
	Hull->BodyInstance.SetMassOverride(HullMassKg, true);
	Hull->BodyInstance.COMNudge = FVector(0.f, 0.f, -30.f); // heavy keel and engine: low centre of mass

	Buoyancy = CreateDefaultSubobject<UBoatBuoyancyComponent>(TEXT("Buoyancy"));
}

void ADelhiFishingBoat::SetBoatInput(float Throttle, float Rudder)
{
	ThrottleInput = FMath::Clamp(Throttle, -0.5f, 1.f);
	RudderInput = FMath::Clamp(Rudder, -1.f, 1.f);
}

void ADelhiFishingBoat::SetCurrentKmh(FVector WorldCurrentKmh)
{
	if (Buoyancy)
	{
		Buoyancy->WaterCurrentKmh = WorldCurrentKmh;
	}
}

float ADelhiFishingBoat::GetGroundSpeedKmh() const
{
	return Hull ? Hull->GetPhysicsLinearVelocity().Size2D() * 0.036f : 0.f;
}

float ADelhiFishingBoat::GetWaterSpeedKmh() const
{
	if (!Hull || !Buoyancy)
	{
		return 0.f;
	}
	const FVector Rel = Hull->GetPhysicsLinearVelocity() - Buoyancy->GetWaterVelocityCmS();
	return Rel.Size2D() * 0.036f;
}

void ADelhiFishingBoat::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Hull || !Buoyancy || !Hull->IsSimulatingPhysics())
	{
		return;
	}

	// Propeller and rudder only work when the stern is in the water.
	const float InWater = FMath::Clamp(Buoyancy->GetSubmergedFraction() * 2.f, 0.f, 1.f);
	if (InWater <= 0.01f)
	{
		return;
	}

	FVector Fwd = Hull->GetForwardVector(); Fwd.Z = 0.f; Fwd = Fwd.GetSafeNormal();
	FVector Right = Hull->GetRightVector(); Right.Z = 0.f; Right = Right.GetSafeNormal();

	const FVector WaterRelCmS = Hull->GetPhysicsLinearVelocity() - Buoyancy->GetWaterVelocityCmS();
	const float VF = FVector::DotProduct(WaterRelCmS, Fwd) * 0.01f;   // m/s through water, forward
	const float VS = FVector::DotProduct(WaterRelCmS, Right) * 0.01f; // m/s sideways

	// Thrust: constant-power curve capped by static thrust. 1 hp = 745.7 W.
	const float PowerW = EnginePowerHp * 745.7f * PropEfficiency;
	const float ThrustN = FMath::Min(MaxStaticThrustN, PowerW / FMath::Max(FMath::Abs(VF), 1.f)) * ThrottleInput; // reverse is clamped to -0.5

	// Quadratic drag sized so full throttle reaches TopSpeedKmh in still water.
	const float VTop = TopSpeedKmh / 3.6f;
	const float DragFwd = (PowerW / VTop) / (VTop * VTop);
	const float DragLat = DragFwd * LateralDragFactor;
	const float FwdForceN = (ThrustN - DragFwd * VF * FMath::Abs(VF)) * InWater;
	const float LatForceN = -DragLat * VS * FMath::Abs(VS) * InWater;

	const FTransform& T = Hull->GetComponentTransform();
	const FVector Stern = T.TransformPosition(SternOffsetCm);
	const FVector Centre = Hull->GetComponentLocation();

	Hull->AddForceAtLocation(Fwd * (FwdForceN * 100.f), Stern);   // N -> kg*cm/s^2
	Hull->AddForceAtLocation(Right * (LatForceN * 100.f), Centre);

	// Rudder side force at the stern: grows with speed through the water, plus prop wash at low speed.
	const float RudderN = RudderInput * (RudderForcePerSpeedSq * VF * VF + 0.25f * FMath::Abs(ThrustN)) * InWater;
	const float Dir = VF >= -0.1f ? 1.f : -1.f;
	Hull->AddForceAtLocation(Right * (-RudderN * Dir * 100.f), Stern);
}
