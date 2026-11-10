#include "Crowd/DhCrowdMassBridge.h"

#include "Crowd/DhCrowdLaneNetwork.h"
#include "Crowd/DhCrowdMass.h"
#include "Engine/World.h"
#include "Mass/EntityFragments.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassEntityTemplate.h"
#include "MassSpawnerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogDhCrowdMass, Log, All);

bool FDhCrowdMassBridge::Init(UWorld* World, const FSoftObjectPath& ConfigPath)
{
	Shutdown();
	if (!World || !ConfigPath.IsValid()) { return false; }

	const UMassEntityConfigAsset* Config = Cast<UMassEntityConfigAsset>(ConfigPath.TryLoad());
	if (!Config)
	{
		UE_LOG(LogDhCrowdMass, Warning, TEXT("Mass entity config '%s' not found; using the pooled backend"), *ConfigPath.ToString());
		return false;
	}

	UMassSpawnerSubsystem* SpawnerSub = World->GetSubsystem<UMassSpawnerSubsystem>();
	UMassEntitySubsystem* EntitySub = World->GetSubsystem<UMassEntitySubsystem>();
	if (!SpawnerSub || !EntitySub)
	{
		UE_LOG(LogDhCrowdMass, Warning, TEXT("Mass subsystems are not available in this world; using the pooled backend"));
		return false;
	}

	const FMassEntityTemplate& T = Config->GetOrCreateEntityTemplate(*World);
	if (!T.IsValid())
	{
		UE_LOG(LogDhCrowdMass, Warning, TEXT("Mass entity template is invalid; check the traits in '%s'"), *ConfigPath.ToString());
		return false;
	}

	WorldPtr = World;
	Spawner = SpawnerSub;
	EntitySubsystem = EntitySub;
	Template = &T;
	bReady = true;
	UE_LOG(LogDhCrowdMass, Log, TEXT("Mass crowd backend ready (config %s)"), *ConfigPath.ToString());
	return true;
}

void FDhCrowdMassBridge::Shutdown()
{
	if (bReady && Spawner.IsValid() && Handles.Num() > 0)
	{
		TArray<FMassEntityHandle> All;
		Handles.GenerateValueArray(All);
		Spawner->DestroyEntities(All);
	}
	Handles.Reset();
	Template = nullptr;
	bReady = false;
}

bool FDhCrowdMassBridge::Spawn(FDhCrowdAgent& Agent)
{
	if (!bReady || !Spawner.IsValid() || !EntitySubsystem.IsValid() || !Template) { return false; }
	if (Handles.Contains(Agent.Id)) { return true; }

	TArray<FMassEntityHandle> Out;
	TSharedPtr<FMassEntityManager::FEntityCreationContext> Ctx = Spawner->SpawnEntities(*Template, 1, Out);
	if (Out.Num() != 1) { return false; }

	FMassEntityManager& EM = EntitySubsystem->GetMutableEntityManager();
	const FMassEntityHandle H = Out[0];
	if (FDhCrowdAgentFragment* F = EM.GetFragmentDataPtr<FDhCrowdAgentFragment>(H))
	{
		F->AgentId = Agent.Id;
		F->ArchetypeIndex = Agent.Arch;
		F->Walker = Agent.Walker;
		F->Motion = Agent.Motion;
		F->Activity = static_cast<uint8>(Agent.Activity);
		F->Tint = Agent.Tint;
		F->Skin = Agent.Skin;
	}
	if (FTransformFragment* TF = EM.GetFragmentDataPtr<FTransformFragment>(H))
	{
		TF->GetMutableTransform().SetLocation(FVector(Agent.Pos.X, Agent.Pos.Y, Agent.GroundZ));
		TF->GetMutableTransform().SetRotation(FRotator(0.f, static_cast<float>(Agent.Dir.Rotation().Yaw), 0.f).Quaternion());
	}
	Ctx.Reset(); // flush observers / initialisers now that the fragments are filled

	Handles.Add(Agent.Id, H);
	Agent.bHasMassEntity = true;
	return true;
}

bool FDhCrowdMassBridge::Pull(FDhCrowdAgent& Agent)
{
	if (!bReady || !EntitySubsystem.IsValid()) { return false; }
	const FMassEntityHandle* H = Handles.Find(Agent.Id);
	if (!H) { return false; }

	FMassEntityManager& EM = EntitySubsystem->GetMutableEntityManager();
	if (!EM.IsEntityValid(*H)) { return false; }

	if (const FDhCrowdAgentFragment* F = EM.GetFragmentDataPtr<FDhCrowdAgentFragment>(*H))
	{
		Agent.Walker = F->Walker;
		Agent.Motion = F->Motion;
		Agent.Activity = static_cast<EDhCrowdActivity>(F->Activity);
	}
	if (const FTransformFragment* TF = EM.GetFragmentDataPtr<FTransformFragment>(*H))
	{
		const FTransform& Tr = TF->GetTransform();
		Agent.Pos = Tr.GetLocation();
		Agent.Dir = Tr.GetRotation().GetForwardVector();
	}
	return true;
}

void FDhCrowdMassBridge::Despawn(FDhCrowdAgent& Agent)
{
	if (FMassEntityHandle* H = Handles.Find(Agent.Id))
	{
		Pull(Agent);
		if (Spawner.IsValid())
		{
			Spawner->DestroyEntities(MakeArrayView(H, 1));
		}
		Handles.Remove(Agent.Id);
	}
	Agent.bHasMassEntity = false;
}

void FDhCrowdMassBridge::SetGroundZ(const FDhCrowdAgent& Agent, float Z)
{
	if (!bReady || !EntitySubsystem.IsValid()) { return; }
	if (const FMassEntityHandle* H = Handles.Find(Agent.Id))
	{
		FMassEntityManager& EM = EntitySubsystem->GetMutableEntityManager();
		if (EM.IsEntityValid(*H))
		{
			if (FTransformFragment* TF = EM.GetFragmentDataPtr<FTransformFragment>(*H))
			{
				FTransform& Tr = TF->GetMutableTransform();
				FVector L = Tr.GetLocation();
				L.Z = Z;
				Tr.SetLocation(L);
			}
		}
	}
}

void FDhCrowdMassBridge::Panic(const FDhCrowdAgent& Agent, const FDhCrowdLaneNetwork& Net, const FVector& From, float Seconds)
{
	if (!bReady || !EntitySubsystem.IsValid()) { return; }
	if (const FMassEntityHandle* H = Handles.Find(Agent.Id))
	{
		FMassEntityManager& EM = EntitySubsystem->GetMutableEntityManager();
		if (EM.IsEntityValid(*H))
		{
			if (FDhCrowdAgentFragment* F = EM.GetFragmentDataPtr<FDhCrowdAgentFragment>(*H))
			{
				FDhCrowdSim::Panic(Net, F->Walker, F->Motion, From, Seconds);
			}
		}
	}
}
