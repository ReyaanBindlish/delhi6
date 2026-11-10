#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

class UInteractableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDayaInteracted, AActor*, Interactor, UInteractableComponent*, Source);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayaFocusChanged, bool, bFocused);

/**
 * Put this on any actor Daya can use: a door, a chai stall, a notice board, an NPC.
 * ADayaCharacter traces from his eyes, finds the component, shows Prompt and calls Interact().
 * Missions later bind OnInteracted (Blueprint or C++); no mission logic lives here.
 */
UCLASS(ClassGroup = (Story), meta = (BlueprintSpawnableComponent))
class OLDDELHI_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractableComponent();

	/** Text for the HUD, e.g. "Buy chai (10 INR)". The key glyph is added by the HUD per device. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText Prompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bEnabled = true;

	/** Max distance (cm) from Daya's eyes; the trace itself is capped by ADayaCharacter::InteractDistance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "50"))
	float MaxUseDistance = 250.f;

	/** Highest wins when several overlap the trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	int32 Priority = 0;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool Interact(AActor* Interactor);

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnDayaInteracted OnInteracted;

	/** Fired when Daya starts / stops looking at this object (for outlines). */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnDayaFocusChanged OnFocusChanged;
};
