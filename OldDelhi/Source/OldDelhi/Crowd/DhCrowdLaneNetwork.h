#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "DhCrowdTypes.h"

/** One walkable street centreline (UE world cm). Authored by Tools/ue/build_zonegraph.py from OSM. */
struct FDhCrowdLane
{
	int32 Id = 0;
	FName Street;                 // street type = row name of DT_StreetDensity_Story.csv
	float WidthCm = 400.f;
	TArray<FVector> Points;
	TArray<float> Cum;            // cumulative length at each point
	float Length = 0.f;
	FIntPoint NodeA = FIntPoint(0, 0);
	FIntPoint NodeB = FIntPoint(0, 0);
};

/** A sample of a lane inside a grid cell (every 5 m). Used to place agents and to measure street length per cell. */
struct FDhLaneSample
{
	int32 Lane = 0;
	float Dist = 0.f;
};

/** Cached view of the player used for avoidance. */
struct FDhCrowdPlayerInfo
{
	bool bValid = false;
	FVector Pos = FVector::ZeroVector;
	FVector Vel = FVector::ZeroVector;
	FVector ViewDir = FVector::ForwardVector;
};

/**
 * Lightweight lane graph: polylines joined at shared endpoints, a 200 m cell index, random-walk successor
 * choice. Loaded from <location>_lanes.json (written by build_zonegraph.py next to the ZoneShape actors it
 * places), so the crowd works with or without the ZoneGraph plugin data being built.
 */
class OLDDELHI_API FDhCrowdLaneNetwork
{
public:
	static constexpr float CellSizeCm = 20000.f;
	static constexpr float SampleStepCm = 500.f;

	bool LoadFromJsonFile(const FString& Path);
	bool LoadFromJsonString(const FString& Json);
	/** For tests / procedural fallback: one straight lane. */
	void AddLane(FName Street, float WidthCm, const TArray<FVector>& Points);
	void Rebuild();
	void Reset();

	bool IsEmpty() const { return Lanes.Num() == 0; }
	int32 NumLanes() const { return Lanes.Num(); }
	const FDhCrowdLane& GetLane(int32 Index) const { return Lanes[Index]; }
	bool IsValidLane(int32 Index) const { return Lanes.IsValidIndex(Index); }
	const FString& GetLocationId() const { return LocationId; }
	const TMap<FIntPoint, TArray<FDhLaneSample>>& GetCells() const { return Cells; }

	static FIntPoint CellOf(const FVector& P)
	{
		return FIntPoint(FMath::FloorToInt(P.X / CellSizeCm), FMath::FloorToInt(P.Y / CellSizeCm));
	}
	static FVector CellCenter(const FIntPoint& C)
	{
		return FVector((C.X + 0.5f) * CellSizeCm, (C.Y + 0.5f) * CellSizeCm, 0.f);
	}

	/** Centreline point and tangent (in +Dist direction) at a distance along a lane. */
	void SampleLane(int32 LaneIdx, float Dist, FVector& OutPos, FVector& OutTangent) const;
	/** World pose of a walker: centreline plus lateral offset to the right of travel; OutDir is the travel direction. */
	void GetPose(const FDhCrowdLaneWalker& W, FVector& OutPos, FVector& OutDir) const;
	/** Moves along the lane, switching lanes at nodes. Returns true if the lane changed. */
	bool Advance(FDhCrowdLaneWalker& W, float DeltaCm, FRandomStream& Rng) const;
	/** Puts a walker on a lane at a distance with a travel direction and a home side. */
	void Place(FDhCrowdLaneWalker& W, int32 LaneIdx, float Dist, int32 Dir, float HomeFraction) const;

private:
	static FIntPoint NodeKey(const FVector& P)
	{
		return FIntPoint(FMath::RoundToInt(P.X / 50.f), FMath::RoundToInt(P.Y / 50.f));
	}

	FString LocationId;
	TArray<FDhCrowdLane> Lanes;
	TMap<FIntPoint, TArray<int32>> NodeLanes;
	TMap<FIntPoint, TArray<FDhLaneSample>> Cells;
};

/** The single step function shared by pooled agents and Mass entities so both behave identically. */
struct OLDDELHI_API FDhCrowdSim
{
	/**
	 * Advances one agent by Dt seconds: dwell, flee, lane following, yielding to / sidestepping the player.
	 * Returns the activity to display and the new pose.
	 */
	static EDhCrowdActivity Step(const FDhCrowdLaneNetwork& Net, FDhCrowdLaneWalker& W, FDhCrowdAgentMotion& M,
		const FDhCrowdPlayerInfo& Player, float Dt, FVector& OutPos, FVector& OutDir);

	/** Makes the agent run away from a point (gunshot, vehicle crash). */
	static void Panic(const FDhCrowdLaneNetwork& Net, FDhCrowdLaneWalker& W, FDhCrowdAgentMotion& M,
		const FVector& From, float Seconds);
};
