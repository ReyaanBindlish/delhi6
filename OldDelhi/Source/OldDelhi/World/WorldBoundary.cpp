#include "World/WorldBoundary.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/SoftObjectPath.h"
#include "World/BoundarySubsystem.h"

AWorldBoundary::AWorldBoundary()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("BoundarySpline"));
	Spline->SetupAttachment(Root);
	Spline->SetClosedLoop(true);
	// Default rectangle (replaced by the level designer or SetPolygonFromLatLon). Linear so corners stay sharp.
	TArray<FVector> Pts = {
		FVector(-DefaultHalfExtentsCm.X, -DefaultHalfExtentsCm.Y, 0.f), FVector(DefaultHalfExtentsCm.X, -DefaultHalfExtentsCm.Y, 0.f),
		FVector(DefaultHalfExtentsCm.X, DefaultHalfExtentsCm.Y, 0.f), FVector(-DefaultHalfExtentsCm.X, DefaultHalfExtentsCm.Y, 0.f) };
	Spline->SetSplinePoints(Pts, ESplineCoordinateSpace::Local, true);
	for (int32 i = 0; i < Pts.Num(); ++i) { Spline->SetSplinePointType(i, ESplinePointType::Linear, false); }
	Spline->UpdateSpline();

	HumAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("Hum"));
	HumAudio->SetupAttachment(Root);
	HumAudio->bAutoActivate = false;
	HumAudio->bAllowSpatialization = true;

	ProximityPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("ProximityPost"));
	ProximityPostProcess->SetupAttachment(Root);
	ProximityPostProcess->bUnbound = true;
	ProximityPostProcess->BlendWeight = 0.f;
	ProximityPostProcess->Priority = 10.f;
	ProximityPostProcess->Settings.bOverride_SceneFringeIntensity = true;
	ProximityPostProcess->Settings.SceneFringeIntensity = 3.f;
}

void AWorldBoundary::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildBoundary();
}

void AWorldBoundary::BeginPlay()
{
	Super::BeginPlay();
	// Transient wall components are not duplicated into PIE / packaged worlds: rebuild if needed.
	if (!bBuiltOnce || BuiltComponents.Num() == 0) { RebuildBoundary(); }
	if (HumSound) { HumAudio->SetSound(HumSound); }
	if (UWorld* W = GetWorld())
	{
		if (UBoundarySubsystem* Sub = W->GetSubsystem<UBoundarySubsystem>()) { Sub->RegisterBoundary(this); }
	}
}

void AWorldBoundary::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		if (UBoundarySubsystem* Sub = W->GetSubsystem<UBoundarySubsystem>()) { Sub->UnregisterBoundary(this); }
	}
	Super::EndPlay(EndPlayReason);
}

// ------------------------------------------------------------------------------------------------ authoring helpers
void AWorldBoundary::SetRectangle(FVector Center, float HalfX, float HalfY)
{
	const FVector L = GetActorTransform().InverseTransformPosition(Center);
	TArray<FVector> Pts = { L + FVector(-HalfX, -HalfY, 0), L + FVector(HalfX, -HalfY, 0), L + FVector(HalfX, HalfY, 0), L + FVector(-HalfX, HalfY, 0) };
	Spline->SetSplinePoints(Pts, ESplineCoordinateSpace::Local, true);
	for (int32 i = 0; i < Pts.Num(); ++i) { Spline->SetSplinePointType(i, ESplinePointType::Linear, false); }
	Spline->UpdateSpline();
	RebuildBoundary();
}

void AWorldBoundary::SetPolygonFromLatLon(const TArray<FVector2D>& LatLon, FVector2D Origin, float GroundZ)
{
	if (LatLon.Num() < 3) { return; }
	const double CosLat = FMath::Cos(FMath::DegreesToRadians(Origin.X));
	TArray<FVector> Pts;
	Pts.Reserve(LatLon.Num());
	for (const FVector2D& P : LatLon)
	{
		const double EastM = (P.Y - Origin.Y) * CosLat * 111320.0;
		const double NorthM = (P.X - Origin.X) * 110540.0;
		Pts.Add(GetActorTransform().InverseTransformPosition(FVector(EastM * 100.0, -NorthM * 100.0, GroundZ)));
	}
	Spline->SetSplinePoints(Pts, ESplineCoordinateSpace::Local, true);
	for (int32 i = 0; i < Pts.Num(); ++i) { Spline->SetSplinePointType(i, ESplinePointType::Linear, false); }
	Spline->UpdateSpline();
	RebuildBoundary();
}

void AWorldBoundary::SnapSplineToGround()
{
	UWorld* W = GetWorld();
	if (!W) { return; }
	FCollisionQueryParams Q(SCENE_QUERY_STAT(BoundarySnap), true, this);
	for (int32 i = 0; i < Spline->GetNumberOfSplinePoints(); ++i)
	{
		FVector P = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		FHitResult Hit;
		if (W->LineTraceSingleByChannel(Hit, P + FVector(0, 0, 200000.f), P - FVector(0, 0, 200000.f), ECC_WorldStatic, Q))
		{
			P.Z = Hit.ImpactPoint.Z;
			Spline->SetLocationAtSplinePoint(i, P, ESplineCoordinateSpace::World, false);
		}
	}
	Spline->UpdateSpline();
	RebuildBoundary();
}

// ------------------------------------------------------------------------------------------------ build
void AWorldBoundary::BuildPolygonCache(const TArray<FVector>& P)
{
	Polygon.Reset();
	for (int32 i = 0; i + 1 < P.Num(); ++i) { Polygon.Add(FVector2D(P[i].X, P[i].Y)); }
}

void AWorldBoundary::RebuildBoundary()
{
	for (TObjectPtr<UPrimitiveComponent>& C : BuiltComponents)
	{
		if (C) { C->DestroyComponent(); }
	}
	BuiltComponents.Reset();
	Polygon.Reset();
	bBuiltOnce = true;

	if (!Spline || Spline->GetNumberOfSplinePoints() < 3) { return; }

	// ---- sample the loop
	const float Len = Spline->GetSplineLength();
	const int32 N = FMath::Max(8, FMath::CeilToInt(Len / FMath::Max(200.f, SegmentLengthCm)));
	TArray<FVector> P;
	P.Reserve(N + 1);
	for (int32 i = 0; i < N; ++i)
	{
		P.Add(Spline->GetLocationAtDistanceAlongSpline(Len * (float)i / (float)N, ESplineCoordinateSpace::World));
	}
	P.Add(P[0]);
	BuildPolygonCache(P);

	// ---- proximity post material (once per rebuild)
	if (ProximityPostMaterial)
	{
		ProximityPostProcess->Settings.WeightedBlendables.Array.Reset();
		ProximityPostProcess->Settings.AddBlendable(ProximityPostMaterial, 1.f);
	}

	UStaticMesh* Mesh = bBuildVisual ? Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad()) : nullptr;
	UMaterialInterface* Mat = (bBuildVisual && Mesh) ? BoundaryMaterial.LoadSynchronous() : nullptr;
	const FTransform ActorXf = GetActorTransform();
	const float Half = WallHeightCm * 0.5f;
	USceneComponent* Root = GetRootComponent();

	for (int32 i = 0; i < N; ++i)
	{
		const FVector A = P[i], B = P[i + 1];
		const FVector D = B - A;
		const float SegLen = D.Size();
		if (SegLen < 1.f) { continue; }
		const FVector T2 = FVector(D.X, D.Y, 0.f).GetSafeNormal();

		// ---- translucent lattice wall (no collision, no shadows)
		if (Mesh && Mat)
		{
			USplineMeshComponent* SM = NewObject<USplineMeshComponent>(this, NAME_None, RF_Transient);
			SM->SetupAttachment(Root);
			SM->SetRelativeLocation(FVector(0.f, 0.f, Half));
			SM->SetMobility(EComponentMobility::Movable);
			SM->SetStaticMesh(Mesh);
			SM->SetForwardAxis(ESplineMeshAxis::X, false);
			const FVector LA = ActorXf.InverseTransformPosition(A) - FVector(0, 0, Half);
			const FVector LB = ActorXf.InverseTransformPosition(B) - FVector(0, 0, Half);
			const FVector Tan = ActorXf.InverseTransformVector(D);
			SM->SetStartAndEnd(LA, Tan, LB, Tan, false);
			// Cube is 100 cm: Y scale = thickness, Z scale = height.
			const FVector2D Scale(0.02f, WallHeightCm / 100.f);
			SM->SetStartScale(Scale, false);
			SM->SetEndScale(Scale, true);
			SM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SM->SetCastShadow(false);
			SM->SetGenerateOverlapEvents(false);
			SM->SetCanEverAffectNavigation(false);
			SM->SetCullDistance(VisibleDistanceCm);
			SM->ComponentTags.Add(TEXT("NoDecal"));

			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, this);
			if (MID)
			{
				// Seg = (anchor X, anchor Y, distance along the wall at the anchor, ground Z); SegDir = unit tangent.
				// M_Boundary rebuilds a continuous along-wall coordinate from these, so the lattice does not tear at joints.
				MID->SetVectorParameterValue(TEXT("Seg"), FLinearColor(A.X, A.Y, Len * (float)i / (float)N, (A.Z + B.Z) * 0.5f));
				MID->SetVectorParameterValue(TEXT("SegDir"), FLinearColor(T2.X, T2.Y, 0.f, 0.f));
				MID->SetScalarParameterValue(TEXT("Cell"), LatticeCellCm);
				SM->SetMaterial(0, MID);
			}
			SM->RegisterComponent();
			BuiltComponents.Add(SM);
		}

		// ---- blocking wall: Pawn, Vehicle, PhysicsBody. Everything else (bullets, camera, visibility) passes.
		if (bBuildCollision)
		{
			UBoxComponent* Box = NewObject<UBoxComponent>(this, NAME_None, RF_Transient);
			Box->SetupAttachment(Root);
			const FVector Mid = (A + B) * 0.5f + FVector(0, 0, Half);
			const FQuat WorldRot = FRotator(0.f, D.Rotation().Yaw, 0.f).Quaternion();
			Box->SetRelativeLocationAndRotation(ActorXf.InverseTransformPosition(Mid), ActorXf.InverseTransformRotation(WorldRot));
			Box->SetBoxExtent(FVector(SegLen * 0.5f + CollisionThicknessCm * 0.5f, CollisionThicknessCm * 0.5f, Half), false);
			Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Box->SetCollisionObjectType(ECC_WorldStatic);
			Box->SetCollisionResponseToAllChannels(ECR_Ignore);
			Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
			Box->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
			Box->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
			Box->SetGenerateOverlapEvents(false);
			Box->SetHiddenInGame(true);
			Box->SetCastShadow(false);
			Box->ComponentTags.Add(TEXT("NoDecal"));
			Box->RegisterComponent();
			BuiltComponents.Add(Box);
		}
	}
}

// ------------------------------------------------------------------------------------------------ queries
float AWorldBoundary::QueryDistance(FVector WorldLocation, FVector& OutClosest, FVector2D& OutInward2D) const
{
	OutClosest = WorldLocation;
	OutInward2D = FVector2D::ZeroVector;
	const int32 Num = Polygon.Num();
	if (Num < 3) { return TNumericLimits<float>::Max(); }

	const FVector2D Pt(WorldLocation.X, WorldLocation.Y);
	double BestSq = TNumericLimits<double>::Max();
	FVector2D BestPoint = Pt;
	bool bInside = false;
	for (int32 i = 0, j = Num - 1; i < Num; j = i++)
	{
		const FVector2D& A = Polygon[j];
		const FVector2D& B = Polygon[i];
		// even-odd inside test
		if (((A.Y > Pt.Y) != (B.Y > Pt.Y)) && (Pt.X < (B.X - A.X) * (Pt.Y - A.Y) / (B.Y - A.Y) + A.X)) { bInside = !bInside; }
		// closest point on edge
		const FVector2D AB = B - A;
		const double L2 = AB.SizeSquared();
		const double T = L2 > 1e-6 ? FMath::Clamp(FVector2D::DotProduct(Pt - A, AB) / L2, 0.0, 1.0) : 0.0;
		const FVector2D C = A + AB * T;
		const double D2 = (Pt - C).SizeSquared();
		if (D2 < BestSq) { BestSq = D2; BestPoint = C; }
	}
	const double D = FMath::Sqrt(BestSq);
	const FVector2D ToWall = BestPoint - Pt;
	const FVector2D Dir = D > 1e-3 ? ToWall / D : FVector2D::ZeroVector;
	OutClosest = FVector(BestPoint.X, BestPoint.Y, WorldLocation.Z);
	OutInward2D = bInside ? -Dir : Dir;
	const float HalfThick = CollisionThicknessCm * 0.5f;
	return bInside ? (float)(D - HalfThick) : (float)(-(D + HalfThick));
}

void AWorldBoundary::UpdateFeedback(float Hum01, const FVector& ClosestPoint, float PostWeight)
{
	if (HumAudio && HumAudio->GetSound())
	{
		if (Hum01 > 0.01f)
		{
			if (!HumAudio->IsPlaying()) { HumAudio->Play(); }
			HumAudio->SetWorldLocation(ClosestPoint);
			HumAudio->SetFloatParameter(TEXT("Proximity"), Hum01);
			HumAudio->SetVolumeMultiplier(Hum01);
		}
		else if (HumAudio->IsPlaying())
		{
			HumAudio->Stop();
		}
	}
	if (ProximityPostProcess) { ProximityPostProcess->BlendWeight = FMath::Clamp(PostWeight, 0.f, 1.f); }
}
