#include "Crowd/DhCrowdMass.h"

#include "Crowd/DhCrowdLaneNetwork.h"
#include "Crowd/DhCrowdSubsystem.h"
#include "Engine/World.h"
#include "Mass/EntityFragments.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassEntityTemplateRegistry.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"

void UDhCrowdAgentTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddFragment<FTransformFragment>();
	BuildContext.AddFragment<FMassVelocityFragment>();
	BuildContext.AddFragment<FDhCrowdAgentFragment>();
	FAgentRadiusFragment& Radius = BuildContext.AddFragment_GetRef<FAgentRadiusFragment>();
	Radius.Radius = AgentRadiusCm;
	BuildContext.AddTag<FDhCrowdAgentTag>();
	if (!bDriveMovement)
	{
		BuildContext.AddTag<FDhCrowdExternalMovementTag>();
	}
}

UDhCrowdMovementProcessor::UDhCrowdMovementProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;
}

void UDhCrowdMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FDhCrowdAgentFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FDhCrowdAgentTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FDhCrowdExternalMovementTag>(EMassFragmentPresence::None);
}

void UDhCrowdMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = Context.GetWorld();
	UDhCrowdSubsystem* Crowd = World ? World->GetSubsystem<UDhCrowdSubsystem>() : nullptr;
	if (!Crowd || Crowd->GetLaneNetwork().IsEmpty() || Crowd->IsPaused())
	{
		return;
	}

	const FDhCrowdLaneNetwork& Net = Crowd->GetLaneNetwork();
	const FDhCrowdPlayerInfo Player = Crowd->GetPlayerInfo();
	const float Dt = FMath::Min(Context.GetDeltaTimeSeconds(), 0.1f);

	EntityQuery.ForEachEntityChunk(Context, [&Net, &Player, Dt](FMassExecutionContext& ChunkContext)
	{
		const TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = ChunkContext.GetMutableFragmentView<FMassVelocityFragment>();
		const TArrayView<FDhCrowdAgentFragment> Agents = ChunkContext.GetMutableFragmentView<FDhCrowdAgentFragment>();
		const int32 Num = ChunkContext.GetNumEntities();

		for (int32 i = 0; i < Num; ++i)
		{
			FDhCrowdAgentFragment& Agent = Agents[i];
			FVector Pos = FVector::ZeroVector;
			FVector Dir = FVector::ForwardVector;
			const EDhCrowdActivity Act = FDhCrowdSim::Step(Net, Agent.Walker, Agent.Motion, Player, Dt, Pos, Dir);
			Agent.Activity = static_cast<uint8>(Act);

			FTransform& Tr = Transforms[i].GetMutableTransform();
			const FVector Old = Tr.GetLocation();
			// XY from the lane; Z is owned by the ground-trace pass in the subsystem.
			Tr.SetLocation(FVector(Pos.X, Pos.Y, Old.Z));
			Tr.SetRotation(FRotator(0.f, static_cast<float>(Dir.Rotation().Yaw), 0.f).Quaternion());

			const bool bMoving = (Act == EDhCrowdActivity::Walk || Act == EDhCrowdActivity::Flee);
			Velocities[i].Value = bMoving ? Dir * Agent.Motion.BaseSpeed * (Act == EDhCrowdActivity::Flee ? 2.4f : 1.f) : FVector::ZeroVector;
		}
	});
}
