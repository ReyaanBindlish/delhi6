#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "MassEntityTypes.h"
#include "MassProcessor.h"
#include "Crowd/DhCrowdTypes.h"
#include "DhCrowdMass.generated.h"

/**
 * Mass Entity side of the crowd (research/05 + 07: Mass entities, ZoneGraph lanes, MassCrowd visualisation).
 * The same lane-following step (FDhCrowdSim) used by the pooled path runs here as a Mass processor, so a
 * far-tier agent can be handed between a Mass entity and a pooled actor without a visible change.
 *
 * Add UDhCrowdAgentTrait to a Mass Entity Config together with the standard "Crowd Visualization" trait
 * (MassCrowd) and, optionally, the MassCrowd ZoneGraph navigation traits (then untick bDriveMovement).
 */

/** Per-agent state carried on the entity. Mirrors FDhCrowdAgent. */
USTRUCT()
struct OLDDELHI_API FDhCrowdAgentFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY() int32 AgentId = -1;
	UPROPERTY() int32 ArchetypeIndex = -1;
	UPROPERTY() FDhCrowdLaneWalker Walker;
	UPROPERTY() FDhCrowdAgentMotion Motion;
	UPROPERTY() uint8 Activity = 0;
	UPROPERTY() FLinearColor Tint = FLinearColor::White;
	UPROPERTY() float Skin = 0.5f;
};

/** Tag on every crowd entity spawned by this system. */
USTRUCT()
struct OLDDELHI_API FDhCrowdAgentTag : public FMassTag
{
	GENERATED_BODY()
};

/** Present when something else (ZoneGraph navigation) moves the entity; the lane-follow processor then skips it. */
USTRUCT()
struct OLDDELHI_API FDhCrowdExternalMovementTag : public FMassTag
{
	GENERATED_BODY()
};

UCLASS(meta = (DisplayName = "Dhurandhar Crowd Agent"))
class OLDDELHI_API UDhCrowdAgentTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	/** Move along the lane network loaded by UDhCrowdSubsystem. Untick when MassCrowd ZoneGraph navigation drives the entity. */
	UPROPERTY(EditAnywhere, Category = "Crowd") bool bDriveMovement = true;
	UPROPERTY(EditAnywhere, Category = "Crowd") float AgentRadiusCm = 35.f;

	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};

/** Lane following, dwell, panic and player avoidance for crowd entities. */
UCLASS()
class OLDDELHI_API UDhCrowdMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UDhCrowdMovementProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
