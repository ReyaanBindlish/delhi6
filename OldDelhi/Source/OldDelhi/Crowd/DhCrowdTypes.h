#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/SoftObjectPath.h"
#include "DhCrowdTypes.generated.h"

/**
 * Shared types of the ambient crowd (research/05: 5000 simulated, ~600 visible, hard cap 750).
 * Everything here is plain data so the same structs are used by the pooled path, the instanced
 * renderer and the Mass fragments.
 */

/** Representation tier of one simulated agent. Higher = more expensive. */
UENUM(BlueprintType)
enum class EDhCrowdTier : uint8
{
	Dormant,       // in the 5000 pool, not in the world (abstract population counters only)
	Abstract,      // simulated on its lane, no mesh (time-sliced, ~1 Hz)
	FarInstanced,  // 60-150 m: low poly instance (VAT or Mass entity)
	NearInstanced, // 25-60 m: ~150 instances
	Skeletal,      // 8-25 m: ~40 reduced-rate skeletal characters
	Hero           // < 8 m: 12 full skeletal characters
};

UENUM(BlueprintType)
enum class EDhCrowdActivity : uint8
{
	Walk,
	Idle,
	Work,  // stationary at a stall/post: idle loop, looks around
	Flee
};

UENUM(BlueprintType)
enum class EDhCrowdBackend : uint8
{
	/** Mass when the config asset resolves and the Mass modules are up, otherwise Pooled. */
	Auto,
	/** Far tiers are Mass entities (ZoneGraph/MassCrowd upgrade path). Falls back to Pooled on failure. */
	Mass,
	/** Pooled ACharacter actors for near tiers, instanced static mesh for far tiers. No Mass needed. */
	Pooled
};

UENUM(BlueprintType)
enum class EDhCrowdRegion : uint8
{
	OldDelhi,
	DelhiNCR,
	Karachi
};

/** Coarse behaviour class parsed from the SpawnGroup column of the archetype tables. */
UENUM(BlueprintType)
enum class EDhCrowdGroup : uint8
{
	Vendor,
	Worker,
	Driver,
	Police,
	Student,
	Commuter,
	Tourist,
	Worshipper,
	Static,
	Special,
	Resident,
	Other
};

/** Position of an agent on the lane network. Lane geometry is the street centreline; Lateral is cm to the right of travel. */
USTRUCT()
struct FDhCrowdLaneWalker
{
	GENERATED_BODY()

	UPROPERTY() int32 Lane = -1;
	UPROPERTY() float Dist = 0.f;
	UPROPERTY() int32 Dir = 1;
	UPROPERTY() float Lateral = 0.f;
	UPROPERTY() float HomeLateral = -80.f;
	UPROPERTY() int32 RngState = 1;
};

/** Per-agent motion parameters, stepped by FDhCrowdSim (identical on the pooled path and in Mass). */
USTRUCT()
struct FDhCrowdAgentMotion
{
	GENERATED_BODY()

	UPROPERTY() float BaseSpeed = 130.f; // cm/s
	UPROPERTY() float DwellLeft = 0.f;
	UPROPERTY() float FleeLeft = 0.f;
	UPROPERTY() bool bStationary = false;
};

/** One of the 5000 pooled agents. Not a UObject: 5000 of them cost about 1 MB. */
struct FDhCrowdAgent
{
	int32 Id = 0;
	EDhCrowdTier Tier = EDhCrowdTier::Dormant;
	EDhCrowdActivity Activity = EDhCrowdActivity::Walk;

	int16 Arch = -1;           // index into the archetype info array
	bool bFemale = false;
	float Scale = 1.f;
	FLinearColor Tint = FLinearColor::White;
	float Skin = 0.5f;         // 0..1 skin-tone variation, hue locked in the material

	FDhCrowdLaneWalker Walker;
	FDhCrowdAgentMotion Motion;

	FVector Pos = FVector::ZeroVector;
	FVector Dir = FVector::ForwardVector;
	float GroundZ = 0.f;
	bool bHaveGround = false;
	double LastSimTime = 0.0;
	float Phase = 0.f;         // gait phase for instanced bobbing

	int32 Slot = INDEX_NONE;   // instanced-mesh slot
	TWeakObjectPtr<class ADhCrowdCharacter> Actor;
	bool bHasMassEntity = false;
};

/** Row-derived info of one archetype (from DT_Archetypes*.csv). */
struct FDhCrowdArchetypeInfo
{
	FName Id;
	FString Label;
	EDhCrowdGroup Group = EDhCrowdGroup::Other;
	int32 AgeMin = 18;
	int32 AgeMax = 60;
	float FemaleShare = 0.5f;  // normalised, other share folded into the nearer binary stand-in
	float BaseSpeedCms = 130.f;
	float StationaryChance = 0.f;
	uint32 PlaceMask = 0;      // bits, see DhCrowdArchetypeBits
	int32 UniformKind = 0;     // 0 civilian palette, 1 khaki police, 2 desert ranger, 3 sanitation, 4 school uniform, 5 security grey
};

namespace DhCrowdArchetypeBits
{
	constexpr uint32 Worship = 1u << 0;
	constexpr uint32 Harbour = 1u << 1;
	constexpr uint32 Station = 1u << 2;
	constexpr uint32 Checkpoint = 1u << 3;
	constexpr uint32 Market = 1u << 4;
	constexpr uint32 Jail = 1u << 5;
	constexpr uint32 Highway = 1u << 6;
	constexpr uint32 School = 1u << 7;
	constexpr uint32 Residential = 1u << 8;
}

/** Where to find a stand-in character. Candidates are tried in order until the mesh loads. */
USTRUCT(BlueprintType)
struct FDhCrowdVisualSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd", meta = (AllowedClasses = "/Script/Engine.SkeletalMesh")) FSoftObjectPath Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd", meta = (AllowedClasses = "/Script/Engine.AnimSequence")) FSoftObjectPath Walk;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd", meta = (AllowedClasses = "/Script/Engine.AnimSequence")) FSoftObjectPath Idle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd") bool bFemale = false;
};

/** Live numbers for HUD / debug / tests. */
USTRUCT(BlueprintType)
struct FDhCrowdStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName Location;
	UPROPERTY(BlueprintReadOnly) float Hours = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 PoolSize = 0;
	UPROPERTY(BlueprintReadOnly) int32 Active = 0;
	UPROPERTY(BlueprintReadOnly) int32 Visible = 0;
	UPROPERTY(BlueprintReadOnly) int32 Hero = 0;
	UPROPERTY(BlueprintReadOnly) int32 Skeletal = 0;
	UPROPERTY(BlueprintReadOnly) int32 NearInstanced = 0;
	UPROPERTY(BlueprintReadOnly) int32 FarInstanced = 0;
	UPROPERTY(BlueprintReadOnly) int32 VisibleCap = 0;
	UPROPERTY(BlueprintReadOnly) float Density = 1.f;
	UPROPERTY(BlueprintReadOnly) bool bMassBackend = false;
	UPROPERTY(BlueprintReadOnly) int32 Lanes = 0;
};
