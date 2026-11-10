#include "Player/InteractableComponent.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Prompt = FText::FromString(TEXT("Use"));
}

bool UInteractableComponent::CanInteract_Implementation(AActor* Interactor) const
{
	return bEnabled && Interactor != nullptr;
}

bool UInteractableComponent::Interact(AActor* Interactor)
{
	if (!CanInteract(Interactor))
	{
		return false;
	}
	OnInteracted.Broadcast(Interactor, this);
	return true;
}
