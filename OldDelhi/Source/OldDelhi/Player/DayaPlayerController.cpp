#include "Player/DayaPlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

ADayaPlayerController::ADayaPlayerController()
{
	DefaultMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_Default.IMC_Default")));
	NotebookAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Notebook.IA_Notebook")));
	PauseAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Pause.IA_Pause")));

	// Keep ticking while paused so Esc / Start can resume without a UI.
	bShouldPerformFullTickWhenPaused = true;
}

void ADayaPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -75.f;
		PlayerCameraManager->ViewPitchMax = 75.f;
	}
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	AddMappingContext();
}

void ADayaPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AddMappingContext(); // idempotent: re-adding replaces the same context
}

void ADayaPlayerController::AddMappingContext()
{
	if (!IsLocalController())
	{
		return;
	}
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (const UInputMappingContext* IMC = DefaultMappingContext.LoadSynchronous())
			{
				Subsystem->AddMappingContext(IMC, MappingPriority);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("IMC_Default missing: run Tools/ue/setup_input.py (see docs/CONTROLS.md)."));
			}
		}
	}
}

void ADayaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (const UInputAction* A = NotebookAction.LoadSynchronous())
		{
			EIC->BindAction(A, ETriggerEvent::Started, this, &ADayaPlayerController::OnNotebookPressed);
		}
		if (const UInputAction* A = PauseAction.LoadSynchronous())
		{
			EIC->BindAction(A, ETriggerEvent::Started, this, &ADayaPlayerController::OnPausePressed);
		}
	}
}

void ADayaPlayerController::OnNotebookPressed()
{
	SetNotebookOpen(!bNotebookOpen);
}

void ADayaPlayerController::SetNotebookOpen(bool bOpen)
{
	if (bNotebookOpen == bOpen)
	{
		return;
	}
	bNotebookOpen = bOpen;
	// While the notebook is open the world keeps running (the city does not wait), but the camera is frozen.
	SetIgnoreLookInput(bOpen);
	OnNotebookToggled.Broadcast(bOpen);
}

void ADayaPlayerController::OnPausePressed()
{
	TogglePause();
}

void ADayaPlayerController::TogglePause()
{
	PauseFrame = GFrameCounter;
	const bool bNowPaused = !IsPaused();
	SetPause(bNowPaused);
	OnPauseToggled.Broadcast(bNowPaused);
}

void ADayaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Enhanced Input bindings do not fire while paused, so poll the resume keys ourselves.
	if (IsPaused() && PauseFrame != GFrameCounter
		&& (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Right)))
	{
		TogglePause();
	}
}
