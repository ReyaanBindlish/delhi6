#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "DayaCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UDayaVitalsComponent;
class UMoneyLedgerComponent;
class UInteractableComponent;

UENUM(BlueprintType)
enum class EDayaGait : uint8
{
	Idle,
	Walk,
	Jog,
	Sprint,
	Crouch
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusedInteractableChanged, UInteractableComponent*, NewFocus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayaGaitChanged, EDayaGait, NewGait);

/**
 * Daya: an ordinary man, not a superhero (bible, "Physics and realism rules").
 *  - Real speeds: walk 1.4 m/s, jog 3 m/s, sprint 5.5 m/s. Sprinting drains stamina; he tires.
 *  - No jump. He can step up kerbs automatically and mantle/vault obstacles between 0.3 and 1.0 m.
 *  - Cough fits (UDayaVitalsComponent) lock movement for 2-4 s; injuries slow him.
 *  - Falling from height reports to the vitals ledger (injury above 3 m, likely death above 6 m).
 * Third-person over-the-shoulder camera with shoulder swap, and a first-person toggle.
 */
UCLASS()
class OLDDELHI_API ADayaCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADayaCharacter();

	// ---- components --------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Story")
	TObjectPtr<UDayaVitalsComponent> Vitals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Story")
	TObjectPtr<UMoneyLedgerComponent> MoneyLedger;

	// ---- input assets (soft paths; created by Tools/ue/setup_input.py) ----------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> SprintAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> WalkAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> InteractAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> VaultAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> ShoulderSwapAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> CameraToggleAction;

	// ---- gait tuning (cm/s) ------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement") float WalkSpeed = 140.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement") float JogSpeed = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement") float SprintSpeed = 550.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement") float CrouchSpeed = 110.f;
	/** How quickly the speed cap eases between gaits (cm/s per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement") float GaitBlendRate = 450.f;
	/** Stamina points per second while sprinting (bible: one meter for running, fighting, climbing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement") float SprintStaminaPerSecond = 10.f;
	/** After running dry, sprint stays locked until stamina recovers to this fraction of max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0", ClampMax = "1"))
	float SprintRecoverFraction = 0.25f;
	/** Gamepad: L3 toggles sprint (holding a clicked stick is awkward). PC: hold Shift. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement") bool bGamepadSprintToggles = true;

	// ---- vault / mantle (cm) ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault") float VaultMinHeight = 35.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault") float VaultMaxHeight = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault") float VaultReach = 80.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vault") float VaultStaminaCost = 6.f;

	// ---- camera ---------------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float ThirdPersonArmLength = 230.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float ShoulderOffset = 55.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float BaseFOV = 85.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") float SprintFOVBonus = 6.f;
	/** Multiplier on top of the per-device scaling applied in IMC_Default (mouse deg/count, stick deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.05", ClampMax = "5")) float LookSensitivity = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera") bool bInvertLookY = false;

	// ---- interaction ----------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction") float InteractDistance = 250.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction") float InteractRadius = 25.f;

	// ---- state queries (for the AnimBP / HUD) ---------------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "Daya") EDayaGait GetGait() const { return Gait; }
	UFUNCTION(BlueprintPure, Category = "Daya") bool IsFirstPerson() const { return bFirstPerson; }
	UFUNCTION(BlueprintPure, Category = "Daya") bool IsVaulting() const { return bVaulting; }
	UFUNCTION(BlueprintPure, Category = "Daya") bool IsSprintLockedByExhaustion() const { return bSprintExhausted; }
	UFUNCTION(BlueprintPure, Category = "Daya") UInteractableComponent* GetFocusedInteractable() const { return FocusedInteractable; }

	UFUNCTION(BlueprintCallable, Category = "Daya") void SetFirstPerson(bool bNewFirstPerson);
	UFUNCTION(BlueprintCallable, Category = "Daya") void SwapShoulder();
	UFUNCTION(BlueprintCallable, Category = "Daya") bool TryVault();
	UFUNCTION(BlueprintCallable, Category = "Daya") void TryInteract();

	UPROPERTY(BlueprintAssignable, Category = "Daya") FOnFocusedInteractableChanged OnFocusedInteractableChanged;
	UPROPERTY(BlueprintAssignable, Category = "Daya") FOnDayaGaitChanged OnGaitChanged;

	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void SprintStarted();
	void SprintCompleted();
	void WalkStarted() { bWalkHeld = true; }
	void WalkCompleted() { bWalkHeld = false; }
	void CrouchPressed();
	void InteractPressed() { TryInteract(); }
	void VaultPressed() { TryVault(); }
	void ShoulderSwapPressed() { SwapShoulder(); }
	void CameraTogglePressed() { SetFirstPerson(!bFirstPerson); }

	bool IsLocked() const;
	void UpdateGait(float DeltaSeconds);
	void UpdateCamera(float DeltaSeconds);
	void UpdateInteractFocus();
	void TickVault(float DeltaSeconds);
	void FinishVault();
	void SetGait(EDayaGait NewGait);

	EDayaGait Gait = EDayaGait::Idle;
	bool bSprintHeld = false;
	bool bSprintExhausted = false;
	bool bSprintToggleMode = false;
	bool bWalkHeld = false;
	bool bFirstPerson = false;
	float ShoulderSide = 1.f; // +1 right shoulder, -1 left
	float CurrentSpeedCap = 300.f;
	float CurrentShoulderY = 0.f;
	float InteractTimer = 0.f;
	FVector2D LastMoveInput = FVector2D::ZeroVector;

	// fall tracking
	float FallPeakZ = 0.f;

	// vault state
	bool bVaulting = false;
	float VaultElapsed = 0.f;
	float VaultDuration = 0.5f;
	FVector VaultStart = FVector::ZeroVector;
	FVector VaultEnd = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UInteractableComponent> FocusedInteractable;
};
