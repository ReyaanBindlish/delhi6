#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DhurandharGameMode.generated.h"

/**
 * Default game mode for DHURANDHAR: THE GAME. Spawns Daya (ADayaCharacter) under ADayaPlayerController.
 * Set in Project Settings > Maps & Modes, or per-level World Settings. Missions attach later.
 */
UCLASS()
class OLDDELHI_API ADhurandharGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADhurandharGameMode();
};
