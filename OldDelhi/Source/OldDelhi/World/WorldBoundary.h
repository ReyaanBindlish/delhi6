#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldBoundary.generated.h"

class USplineComponent;
class UAudioComponent;
class UPostProcessComponent;
class UPrimitiveComponent;
class UMaterialInterface;
class USoundBase;

/**
 * The construct wall (implementation.md 4.2): the edge of the playable city is invisible until the player is close,
 * then a pale hex lattice shimmers into view. This actor owns
 *  - a closed spline around the playable box (edit its points to follow real streets),
 *  - a translucent, collision-free spline-mesh wall using M_Boundary (hex reveal near the player),
 *  - a matching blocking wall for Pawn, Vehicle and PhysicsBody channels (bullets and cameras pass through; tagged NoDecal),
 *  - an optional hum (MetaSound) and a proximity post-process (chromatic offset, optional scanline material).
 * UBoundarySubsystem (20 Hz) writes MPC_Boundary, pushes players back and fires the touch event.
 */
UCLASS()
class OLDDELHI_API AWorldBoundary : public AActor
{
	GENERATED_BODY()

public:
	AWorldBoundary();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ---------------------------------------------------------------- authoring
	/** Rebuild wall meshes and collision from the spline. Runs automatically on construction. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Boundary")
	void RebuildBoundary();

	/** Drop every spline point onto the terrain below it (editor helper, uses a WorldStatic line trace). */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Boundary")
	void SnapSplineToGround();

	/** Replace the spline with a rectangle centred on `Center` (world cm). */
	UFUNCTION(BlueprintCallable, Category = "Boundary")
	void SetRectangle(FVector Center, float HalfExtentX, float HalfExtentY);

	/**
	 * Replace the spline with a polygon given as (latitude, longitude) vertices, e.g. traced along Netaji Subhash Marg,
	 * the railway line, S P Mukherjee Marg and Asaf Ali Road. Equirectangular around OriginLatLon; project frame is
	 * X east, Y south, Z up, centimetres. GroundZ is the height of the points.
	 */
	UFUNCTION(BlueprintCallable, Category = "Boundary")
	void SetPolygonFromLatLon(const TArray<FVector2D>& LatLonVertices, FVector2D OriginLatLon, float GroundZ = 0.f);

	// ---------------------------------------------------------------- queries
	/**
	 * Distance from a world location to the wall FACE in cm: positive inside the playable area, negative outside.
	 * OutInward2D is the unit direction pointing back towards the city; OutClosest the nearest point on the wall line.
	 */
	UFUNCTION(BlueprintPure, Category = "Boundary")
	float QueryDistance(FVector WorldLocation, FVector& OutClosest, FVector2D& OutInward2D) const;

	bool HasPolygon() const { return Polygon.Num() >= 3; }

	/** Called by UBoundarySubsystem. */
	void UpdateFeedback(float Hum01, const FVector& ClosestPoint, float PostWeight);

	// ---------------------------------------------------------------- components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boundary") TObjectPtr<USplineComponent> Spline;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boundary") TObjectPtr<UAudioComponent> HumAudio;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boundary") TObjectPtr<UPostProcessComponent> ProximityPostProcess;

	// ---------------------------------------------------------------- settings
	/** Half-size of the default rectangle used when the actor is first placed (cm). 120000 = a 2.4 km box. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	FVector2D DefaultHalfExtentsCm = FVector2D(120000.0, 120000.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary", meta = (ClampMin = "500"))
	float WallHeightCm = 3000.f;

	/** Target length of one wall segment (cm). Shorter follows curves better but costs more components. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary", meta = (ClampMin = "200"))
	float SegmentLengthCm = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	float CollisionThicknessCm = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	bool bBuildVisual = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	bool bBuildCollision = true;

	/** Lattice cell size in cm (implementation.md: 60 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	float LatticeCellCm = 60.f;

	/** Wall segments farther than this from the camera are culled (the wall is invisible from afar by design). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	float VisibleDistanceCm = 5000.f;

	/** Created by setup_lighting.py (/Game/Delhi/Materials/M_Boundary). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary")
	TSoftObjectPtr<UMaterialInterface> BoundaryMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Delhi/Materials/M_Boundary.M_Boundary")));

	// ---------------------------------------------------------------- feel (read by UBoundarySubsystem)
	/** Gentle push-back starts this far from the wall face (cm). Spec: 2 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary|Feel")
	float PushStartDistanceCm = 200.f;

	/** Peak push acceleration at the face, cm/s^2 (characters). Vehicles scale it by mass and speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary|Feel")
	float PushAccelCmS2 = 1400.f;

	/** Glitch sting + rumble distance from the face (cm). Spec: 0.5 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary|Feel")
	float TouchDistanceCm = 50.f;

	/** Hum is audible within this distance (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary|Feel")
	float HumRangeCm = 1500.f;

	/** Post effect starts this far from the wall (cm). Spec: 3 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary|Feel")
	float PostEffectRangeCm = 300.f;

	/** Looping MetaSound/Sound Cue. A float input named "Proximity" (0..1) is driven if present. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary|Feel")
	TObjectPtr<USoundBase> HumSound;

	/** Optional post-process material (scanlines) blended in with proximity weight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary|Feel")
	TObjectPtr<UMaterialInterface> ProximityPostMaterial;

private:
	void BuildPolygonCache(const TArray<FVector>& WorldPoints);

	/** World-space XY loop, one vertex per wall segment. */
	TArray<FVector2D> Polygon;

	UPROPERTY(Transient) TArray<TObjectPtr<UPrimitiveComponent>> BuiltComponents;
	bool bBuiltOnce = false;
};
