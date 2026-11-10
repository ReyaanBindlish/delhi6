#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "DayaPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNotebookToggled, bool, bOpen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPauseToggled, bool, bPaused);

/**
 * Owns the Enhanced Input mapping context (IMC_Default: keyboard+mouse AND gamepad), the notebook
 * and pause toggles, and sane pitch limits. Movement/camera actions are bound on ADayaCharacter.
 */
UCLASS()
class OLDDELHI_API ADayaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADayaPlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> NotebookAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	int32 MappingPriority = 0;

	/** Notebook (bible: Daya's notebook is the quest log / ledger). The UI listens to OnNotebookToggled. */
	UFUNCTION(BlueprintCallable, Category = "Daya") void SetNotebookOpen(bool bOpen);
	UFUNCTION(BlueprintPure, Category = "Daya") bool IsNotebookOpen() const { return bNotebookOpen; }
	UFUNCTION(BlueprintCallable, Category = "Daya") void TogglePause();

	UPROPERTY(BlueprintAssignable, Category = "Daya") FOnNotebookToggled OnNotebookToggled;
	UPROPERTY(BlueprintAssignable, Category = "Daya") FOnPauseToggled OnPauseToggled;

	virtual void PlayerTick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	void AddMappingContext();
	void OnNotebookPressed();
	void OnPausePressed();

	bool bNotebookOpen = false;
	uint64 PauseFrame = 0;
};
