#include "Player/DayaCharacter.h"

#include "Player/InteractableComponent.h"
#include "DayaVitalsComponent.h"
#include "MoneyLedgerComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "CollisionQueryParams.h"

namespace
{
	TSoftObjectPtr<UInputAction> InputPath(const TCHAR* AssetName)
	{
		return TSoftObjectPtr<UInputAction>(FSoftObjectPath(FString::Printf(TEXT("/Game/Input/%s.%s"), AssetName, AssetName)));
	}

	constexpr float TPArmHeight = 66.f;  // shoulder-ish pivot above capsule centre
	constexpr float FPArmHeight = 72.f;  // eye height above capsule centre
}

ADayaCharacter::ADayaCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Daya is ~1.70 m tall: capsule 34 cm radius, 88 cm half height.
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	BaseEyeHeight = 72.f;
	CrouchedEyeHeight = 44.f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->bOrientRotationToMovement = true;
	CMC->RotationRate = FRotator(0.f, 380.f, 0.f);
	CMC->MaxWalkSpeed = JogSpeed;
	CMC->MaxWalkSpeedCrouched = CrouchSpeed;
	CMC->MaxAcceleration = 800.f;
	CMC->BrakingDecelerationWalking = 1400.f;
	CMC->GroundFriction = 8.f;
	CMC->MaxStepHeight = 30.f;
	CMC->SetWalkableFloorAngle(46.f);
	CMC->SetCrouchedHalfHeight(58.f);
	CMC->NavAgentProps.bCanCrouch = true;
	CMC->bCanWalkOffLedgesWhenCrouching = true;
	CMC->JumpZVelocity = 0.f;          // no jumping as gameplay
	CMC->AirControl = 0.05f;
	CMC->GravityScale = 1.f;           // real gravity; Chaos world gravity stays -980 cm/s^2
	CMC->MinAnalogWalkSpeed = 40.f;
	CMC->bUseControllerDesiredRotation = false;

	// Mesh: Blueprint subclass assigns the MetaHuman / body mesh; these are UE mannequin-style offsets.
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, TPArmHeight));
	CameraBoom->TargetArmLength = ThirdPersonArmLength;
	CameraBoom->SocketOffset = FVector(0.f, ShoulderOffset, 15.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 14.f;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 16.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(BaseFOV);

	Vitals = CreateDefaultSubobject<UDayaVitalsComponent>(TEXT("Vitals"));
	MoneyLedger = CreateDefaultSubobject<UMoneyLedgerComponent>(TEXT("MoneyLedger"));

	MoveAction = InputPath(TEXT("IA_Move"));
	LookAction = InputPath(TEXT("IA_Look"));
	SprintAction = InputPath(TEXT("IA_Sprint"));
	WalkAction = InputPath(TEXT("IA_Walk"));
	CrouchAction = InputPath(TEXT("IA_Crouch"));
	InteractAction = InputPath(TEXT("IA_Interact"));
	VaultAction = InputPath(TEXT("IA_Vault"));
	ShoulderSwapAction = InputPath(TEXT("IA_ShoulderSwap"));
	CameraToggleAction = InputPath(TEXT("IA_CameraToggle"));

	CurrentSpeedCap = JogSpeed;
	CurrentShoulderY = ShoulderOffset;
}

void ADayaCharacter::BeginPlay()
{
	Super::BeginPlay();
	CurrentSpeedCap = JogSpeed;
	CurrentShoulderY = ShoulderOffset * ShoulderSide;
	FallPeakZ = GetActorLocation().Z;
}

// ------------------------------------------------------------------------------------------------
// Input binding
// ------------------------------------------------------------------------------------------------

void ADayaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("ADayaCharacter: project must use EnhancedInputComponent (Project Settings > Input > Default Classes)."));
		return;
	}

	if (const UInputAction* A = MoveAction.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Triggered, this, &ADayaCharacter::Move);
	}
	if (const UInputAction* A = LookAction.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Triggered, this, &ADayaCharacter::Look);
	}
	if (const UInputAction* A = SprintAction.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &ADayaCharacter::SprintStarted);
		EIC->BindAction(A, ETriggerEvent::Completed, this, &ADayaCharacter::SprintCompleted);
		EIC->BindAction(A, ETriggerEvent::Canceled, this, &ADayaCharacter::SprintCompleted);
	}
	if (const UInputAction* A = WalkAction.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &ADayaCharacter::WalkStarted);
		EIC->BindAction(A, ETriggerEvent::Completed, this, &ADayaCharacter::WalkCompleted);
		EIC->BindAction(A, ETriggerEvent::Canceled, this, &ADayaCharacter::WalkCompleted);
	}
	if (const UInputAction* A = CrouchAction.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &ADayaCharacter::CrouchPressed);
	}
	if (const UInputAction* A = InteractAction.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &ADayaCharacter::InteractPressed);
	}
	if (const UInputAction* A = VaultAction.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &ADayaCharacter::VaultPressed);
	}
	if (const UInputAction* A = ShoulderSwapAction.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &ADayaCharacter::ShoulderSwapPressed);
	}
	if (const UInputAction* A = CameraToggleAction.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &ADayaCharacter::CameraTogglePressed);
	}
}

bool ADayaCharacter::IsLocked() const
{
	return bVaulting || (Vitals && Vitals->IsInputLocked());
}

void ADayaCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller || IsLocked())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(Fwd, Axis.Y);
	AddMovementInput(Right, Axis.X);
	LastMoveInput = Axis;
}

void ADayaCharacter::Look(const FInputActionValue& Value)
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC || PC->IsLookInputIgnored())
	{
		return;
	}
	// Device scaling (mouse deg/count, stick deg/s * dt) lives in IMC_Default; here only the user multiplier.
	// We write the control rotation directly so legacy PlayerController input scales cannot skew it.
	const FVector2D L = Value.Get<FVector2D>() * LookSensitivity;
	FRotator R = PC->GetControlRotation();
	const float Pitch = FMath::Clamp(FRotator::NormalizeAxis(R.Pitch) + (bInvertLookY ? -L.Y : L.Y), -75.f, 75.f);
	R.Pitch = Pitch;
	R.Yaw = FRotator::NormalizeAxis(R.Yaw + L.X);
	R.Roll = 0.f;
	PC->SetControlRotation(R);
}

void ADayaCharacter::SprintStarted()
{
	bool bToggle = false;
	if (bGamepadSprintToggles)
	{
		if (const APlayerController* PC = Cast<APlayerController>(Controller))
		{
			bToggle = PC->WasInputKeyJustPressed(EKeys::Gamepad_LeftThumbstick);
		}
	}
	bSprintHeld = bToggle ? !bSprintHeld : true;
	if (bSprintHeld && bIsCrouched)
	{
		UnCrouch();
	}
	bSprintToggleMode = bToggle;
}

void ADayaCharacter::SprintCompleted()
{
	if (!bSprintToggleMode)
	{
		bSprintHeld = false;
	}
}

void ADayaCharacter::CrouchPressed()
{
	if (IsLocked())
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		bSprintHeld = false;
		Crouch();
	}
}

// ------------------------------------------------------------------------------------------------
// Tick: gait, camera, interaction, vault
// ------------------------------------------------------------------------------------------------

void ADayaCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetCharacterMovement()->IsFalling())
	{
		FallPeakZ = FMath::Max(FallPeakZ, GetActorLocation().Z);
	}

	if (bVaulting)
	{
		TickVault(DeltaSeconds);
	}
	else
	{
		UpdateGait(DeltaSeconds);
	}

	UpdateCamera(DeltaSeconds);

	InteractTimer -= DeltaSeconds;
	if (InteractTimer <= 0.f)
	{
		InteractTimer = 0.1f;
		UpdateInteractFocus();
	}

	LastMoveInput = FVector2D::ZeroVector;
}

void ADayaCharacter::SetGait(EDayaGait NewGait)
{
	if (Gait != NewGait)
	{
		Gait = NewGait;
		OnGaitChanged.Broadcast(Gait);
	}
}

void ADayaCharacter::UpdateGait(float DeltaSeconds)
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	const bool bLocked = Vitals && Vitals->IsInputLocked();
	const bool bHasInput = LastMoveInput.SizeSquared() > 0.01f && !bLocked;

	if (bLocked)
	{
		// A cough fit also ends any sprint.
		bSprintHeld = false;
	}

	// Exhaustion hysteresis: after hitting zero, no sprint until stamina recovers.
	if (Vitals && bSprintExhausted)
	{
		if (Vitals->GetStamina() >= SprintRecoverFraction * Vitals->GetMaxStamina())
		{
			bSprintExhausted = false;
		}
	}

	float BaseSpeed = JogSpeed;
	EDayaGait Target = EDayaGait::Jog;

	if (bIsCrouched)
	{
		BaseSpeed = CrouchSpeed;
		Target = EDayaGait::Crouch;
	}
	else if (bSprintHeld && bHasInput && !bSprintExhausted && CMC->IsMovingOnGround())
	{
		const bool bCanSpend = !Vitals || Vitals->SpendStamina(SprintStaminaPerSecond * DeltaSeconds);
		if (bCanSpend && (!Vitals || Vitals->GetStamina() > 0.f))
		{
			BaseSpeed = SprintSpeed;
			Target = EDayaGait::Sprint;
		}
		else
		{
			bSprintExhausted = true;
			bSprintHeld = false;
		}
	}
	else if (bWalkHeld)
	{
		BaseSpeed = WalkSpeed;
		Target = EDayaGait::Walk;
	}

	const float Multiplier = Vitals ? FMath::Clamp(Vitals->GetMoveSpeedMultiplier(), 0.1f, 1.f) : 1.f;
	CurrentSpeedCap = FMath::FInterpConstantTo(CurrentSpeedCap, BaseSpeed * Multiplier, DeltaSeconds, GaitBlendRate);
	CMC->MaxWalkSpeed = CurrentSpeedCap;
	CMC->MaxWalkSpeedCrouched = CurrentSpeedCap;

	if (!bHasInput && CMC->Velocity.SizeSquared2D() < 100.f)
	{
		if (!bIsCrouched)
		{
			Target = EDayaGait::Idle;
		}
		if (bSprintToggleMode)
		{
			bSprintHeld = false; // a toggled sprint ends when he stops
		}
	}
	SetGait(Target);
}

void ADayaCharacter::UpdateCamera(float DeltaSeconds)
{
	const float TargetArm = bFirstPerson ? 0.f : ThirdPersonArmLength;
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArm, DeltaSeconds, 9.f);

	CurrentShoulderY = FMath::FInterpTo(CurrentShoulderY, ShoulderOffset * ShoulderSide, DeltaSeconds, 8.f);
	const FVector TargetSocket = bFirstPerson ? FVector(14.f, 0.f, 0.f) : FVector(0.f, CurrentShoulderY, 15.f);
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocket, DeltaSeconds, 9.f);

	const float TargetZ = bFirstPerson ? FPArmHeight : TPArmHeight;
	FVector Rel = CameraBoom->GetRelativeLocation();
	Rel.Z = FMath::FInterpTo(Rel.Z, TargetZ, DeltaSeconds, 9.f);
	CameraBoom->SetRelativeLocation(Rel);

	const float TargetFOV = BaseFOV + (Gait == EDayaGait::Sprint ? SprintFOVBonus : 0.f);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaSeconds, 5.f));
}

void ADayaCharacter::SetFirstPerson(bool bNewFirstPerson)
{
	bFirstPerson = bNewFirstPerson;
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	// First person: body follows the camera yaw and strafes. Third person: body turns toward travel direction.
	bUseControllerRotationYaw = bFirstPerson;
	CMC->bOrientRotationToMovement = !bFirstPerson;
	CameraBoom->bEnableCameraLag = !bFirstPerson;
	CameraBoom->bDoCollisionTest = !bFirstPerson;
	if (GetMesh())
	{
		GetMesh()->SetOwnerNoSee(bFirstPerson);
	}
}

void ADayaCharacter::SwapShoulder()
{
	ShoulderSide = -ShoulderSide;
}

// ------------------------------------------------------------------------------------------------
// Falls
// ------------------------------------------------------------------------------------------------

void ADayaCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	if (GetCharacterMovement()->MovementMode == MOVE_Falling && PrevMovementMode != MOVE_Falling)
	{
		FallPeakZ = GetActorLocation().Z;
	}
}

void ADayaCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	const float HeightMeters = (FallPeakZ - GetActorLocation().Z) / 100.f;
	FallPeakZ = GetActorLocation().Z;
	if (Vitals && HeightMeters >= 1.f)
	{
		Vitals->ApplyFall(HeightMeters);
	}
}

// ------------------------------------------------------------------------------------------------
// Interaction
// ------------------------------------------------------------------------------------------------

void ADayaCharacter::UpdateInteractFocus()
{
	UInteractableComponent* NewFocus = nullptr;

	if (Controller && !bVaulting)
	{
		FVector CamLoc;
		FRotator CamRot;
		Controller->GetPlayerViewPoint(CamLoc, CamRot);
		const FVector CamFwd = CamRot.Vector();
		const FVector Eye = GetPawnViewLocation();

		// Start the trace level with Daya's eyes, along the camera ray, so the crosshair is honest in third person.
		const float AlongRay = FMath::Max(0.f, FVector::DotProduct(Eye - CamLoc, CamFwd));
		const FVector Start = CamLoc + CamFwd * AlongRay;
		const FVector End = Start + CamFwd * InteractDistance;

		FCollisionQueryParams Params(SCENE_QUERY_STAT(DayaInteract), false, this);
		FHitResult Hit;
		if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
			FCollisionShape::MakeSphere(InteractRadius), Params))
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				if (UInteractableComponent* Comp = HitActor->FindComponentByClass<UInteractableComponent>())
				{
					const float Dist = FVector::Dist(Eye, Hit.ImpactPoint);
					if (Dist <= Comp->MaxUseDistance && Comp->CanInteract(this))
					{
						NewFocus = Comp;
					}
				}
			}
		}
	}

	if (NewFocus != FocusedInteractable)
	{
		if (FocusedInteractable)
		{
			FocusedInteractable->OnFocusChanged.Broadcast(false);
		}
		FocusedInteractable = NewFocus;
		if (FocusedInteractable)
		{
			FocusedInteractable->OnFocusChanged.Broadcast(true);
		}
		OnFocusedInteractableChanged.Broadcast(FocusedInteractable);
	}
}

void ADayaCharacter::TryInteract()
{
	if (IsLocked())
	{
		return;
	}
	if (FocusedInteractable)
	{
		FocusedInteractable->Interact(this);
	}
}

// ------------------------------------------------------------------------------------------------
// Vault / mantle (obstacles 0.35 - 1.0 m). Not a parkour move: slow, costs stamina, can be refused.
// ------------------------------------------------------------------------------------------------

bool ADayaCharacter::TryVault()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (bVaulting || IsLocked() || bIsCrouched || !CMC->IsMovingOnGround())
	{
		return false;
	}

	const UCapsuleComponent* Cap = GetCapsuleComponent();
	const float R = Cap->GetScaledCapsuleRadius();
	const float HH = Cap->GetScaledCapsuleHalfHeight();
	const FVector Loc = GetActorLocation();
	const float FootZ = Loc.Z - HH;

	// Direction: where the player is steering, else where the body faces.
	FVector Dir = GetActorForwardVector();
	const FVector Accel = CMC->GetCurrentAcceleration();
	if (Accel.SizeSquared2D() > 1.f)
	{
		Dir = Accel.GetSafeNormal2D();
	}
	Dir.Z = 0.f;
	Dir = Dir.GetSafeNormal();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DayaVault), false, this);

	// 1. Is there a wall in front at the minimum vault height?
	const FVector WallStart(Loc.X, Loc.Y, FootZ + VaultMinHeight);
	const FVector WallEnd = WallStart + Dir * (R + VaultReach);
	FHitResult WallHit;
	if (!GetWorld()->LineTraceSingleByChannel(WallHit, WallStart, WallEnd, ECC_Visibility, Params) || WallHit.bStartPenetrating)
	{
		return false;
	}

	// 2. Find the top surface just beyond the face.
	FVector Inward = -WallHit.ImpactNormal;
	Inward.Z = 0.f;
	Inward = Inward.GetSafeNormal();
	if (Inward.IsNearlyZero())
	{
		Inward = Dir;
	}
	const FVector TopXY = WallHit.ImpactPoint + Inward * (R + 5.f);
	const FVector DownStart(TopXY.X, TopXY.Y, FootZ + VaultMaxHeight + 20.f);
	const FVector DownEnd(TopXY.X, TopXY.Y, FootZ + 5.f);
	FHitResult TopHit;
	if (!GetWorld()->LineTraceSingleByChannel(TopHit, DownStart, DownEnd, ECC_Visibility, Params)
		|| TopHit.bStartPenetrating || TopHit.ImpactNormal.Z < 0.7f)
	{
		return false;
	}

	const float ObstacleHeight = TopHit.ImpactPoint.Z - FootZ;
	if (ObstacleHeight < VaultMinHeight || ObstacleHeight > VaultMaxHeight)
	{
		return false;
	}

	// 3. Is there room to stand there?
	const FVector End(TopHit.ImpactPoint.X, TopHit.ImpactPoint.Y, TopHit.ImpactPoint.Z + HH + 2.f);
	if (GetWorld()->OverlapBlockingTestByChannel(End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(R, HH), Params))
	{
		return false;
	}

	// 4. It costs stamina; a winded man cannot do it.
	if (Vitals && !Vitals->SpendStamina(VaultStaminaCost))
	{
		return false;
	}

	bVaulting = true;
	bSprintHeld = false;
	VaultElapsed = 0.f;
	VaultDuration = 0.45f + (ObstacleHeight / 100.f) * 0.35f;
	VaultStart = Loc;
	VaultEnd = End;
	CMC->StopMovementImmediately();
	CMC->SetMovementMode(MOVE_None);
	SetActorRotation(FRotator(0.f, Dir.Rotation().Yaw, 0.f));
	return true;
}

void ADayaCharacter::TickVault(float DeltaSeconds)
{
	VaultElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(VaultElapsed / VaultDuration, 0.f, 1.f);
	const float Smooth = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	FVector P = FMath::Lerp(VaultStart, VaultEnd, Smooth);
	P.Z += FMath::Sin(Alpha * PI) * 18.f; // hands-and-knees hump
	SetActorLocation(P, false, nullptr, ETeleportType::None);
	if (Alpha >= 1.f)
	{
		FinishVault();
	}
}

void ADayaCharacter::FinishVault()
{
	bVaulting = false;
	SetActorLocation(VaultEnd, false, nullptr, ETeleportType::None);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	FallPeakZ = GetActorLocation().Z;
}

