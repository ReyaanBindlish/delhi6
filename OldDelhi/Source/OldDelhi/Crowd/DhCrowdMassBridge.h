#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityHandle.h"
#include "MassEntityTypes.h"
#include "Crowd/DhCrowdTypes.h"

class UMassSpawnerSubsystem;
class UMassEntitySubsystem;
class UWorld;
struct FMassEntityTemplate;

/**
 * Thin, non-UObject adapter between UDhCrowdSubsystem and Mass. The subsystem owns the 5000 agents; this
 * class turns a visible far-tier agent into a Mass entity (and back) so MassCrowd visualisation, LOD and
 * the lane-follow processor handle the 100-400 mid/far agents. Every call is a no-op / returns false when
 * Mass is not ready, which is how the subsystem falls back to the pooled backend.
 */
class FDhCrowdMassBridge
{
public:
	/** Resolves the entity config asset, its template and the Mass subsystems. False = use the pooled backend. */
	bool Init(UWorld* World, const FSoftObjectPath& ConfigPath);
	bool IsReady() const { return bReady; }
	void Shutdown();

	/** Spawns an entity from the agent's state at its current position. */
	bool Spawn(FDhCrowdAgent& Agent);
	/** Copies lane/dwell state back into the agent and destroys the entity. */
	void Despawn(FDhCrowdAgent& Agent);
	/** Pulls position and lane state from the entity into the agent. */
	bool Pull(FDhCrowdAgent& Agent);
	/** Writes the ground height (cm) into the entity transform. */
	void SetGroundZ(const FDhCrowdAgent& Agent, float Z);
	/** Applies a panic to the entity (runs from a gunshot). */
	void Panic(const FDhCrowdAgent& Agent, const class FDhCrowdLaneNetwork& Net, const FVector& From, float Seconds);

	int32 NumEntities() const { return Handles.Num(); }

private:
	bool bReady = false;
	TWeakObjectPtr<UWorld> WorldPtr;
	TWeakObjectPtr<UMassSpawnerSubsystem> Spawner;
	TWeakObjectPtr<UMassEntitySubsystem> EntitySubsystem;
	const FMassEntityTemplate* Template = nullptr;
	TMap<int32, FMassEntityHandle> Handles;
};
