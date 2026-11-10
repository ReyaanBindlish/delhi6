#include "Player/DhurandharGameMode.h"

#include "Player/DayaCharacter.h"
#include "Player/DayaPlayerController.h"

ADhurandharGameMode::ADhurandharGameMode()
{
	DefaultPawnClass = ADayaCharacter::StaticClass();
	PlayerControllerClass = ADayaPlayerController::StaticClass();
}
