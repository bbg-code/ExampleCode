// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#include "Interactions/RCIconInteractionComponent.h"
#include "Interactions/RCIconInteractionViewModel.h"
#include "Interactions/RCInteractorComponent.h"

URCIconInteractionComponent::URCIconInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.0f;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_DuringPhysics;
}

void URCIconInteractionComponent::TryToInteract(URCInteractorComponent* interactor)
{
	if (!IsValid(interactor))
		return;

	if (ActiveInteractions.Contains(interactor))
		return;

	APawn* pawn = Cast<APawn>(interactor->GetOwner());
	if (!pawn)
		return;

	APlayerController* controller = Cast<APlayerController>(pawn->GetController());
	if (!controller)
		return;

	ActiveInteractions.Add(interactor, FRCIconInteractionState(controller));

	SetComponentTickEnabled(true);
}

void URCIconInteractionComponent::CancelInteraction(URCInteractorComponent* interactor)
{
	CanceledInteractions.Add(interactor);

	SetComponentTickEnabled(true);
}

FRCIconInteractionState* URCIconInteractionComponent::GetInteractorState(APlayerController* controller)
{
	APawn* pawn = controller->GetPawn();
	if (!pawn)
		return nullptr;
	URCInteractorComponent* interactor = pawn->GetComponentByClass<URCInteractorComponent>();
	if (!interactor)
		return nullptr;
	FRCIconInteractionState* state = ActiveInteractions.Find(interactor);
	if (!state)
		return nullptr;
	if (state->Controller != controller)
		return nullptr;
	return state;
}

bool URCIconInteractionComponent::CanInteractWith(URCInteractorComponent* interactor) const
{
	if (Enabled == false)
		return false;

	if (!IsValid(interactor))
		return false;

	APawn* pawn = Cast<APawn>(interactor->GetOwner());
	if (!IsValid(pawn))
		return false;

	APlayerController* controller = Cast<APlayerController>(pawn->Controller);
	if (!IsValid(controller))
		return false;

	return (GetComponentLocation() - pawn->GetActorLocation()).SizeSquared() <
		FMath::Square(InteractionRange);
}

void URCIconInteractionComponent::UpdateViewModels(APlayerController* viewer, float distanceToViewer, bool focused)
{
	Super::UpdateViewModels(viewer, distanceToViewer, focused);

	// on the first update, before the subsystem creates the widget, see SetViewModels
	if (!InteractionViewModel)
	{
		InteractionViewModel = NewObject<URCIconInteractionViewModel>(this);
	}

	// only the focused icon's interaction can be started, see URCInteractorComponent
	APawn* pawn = focused ? viewer->GetPawn() : nullptr;
	URCInteractorComponent* interactor = pawn ? pawn->GetComponentByClass<URCInteractorComponent>() : nullptr;
	const bool canInteract = CanInteractWith(interactor);

	float progress = 0.0f;
	if (canInteract)
	{
		if (const FRCIconInteractionState* state = ActiveInteractions.Find(interactor))
		{
			progress = (InteractionTime > 0.0f) ? FMath::Clamp(state->Time / InteractionTime, 0.0f, 1.0f) : 1.0f;
		}
	}
	InteractionViewModel->SetInteractionVisibility(canInteract ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	InteractionViewModel->SetInteractionProgress(progress);
}

void URCIconInteractionComponent::SetViewModels()
{
	Super::SetViewModels();
	GetIconWidget().SetViewModel(InteractionViewModel);
}

void URCIconInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (auto& [interactorRef, interactionState] : ActiveInteractions)
	{
		if (!CanInteractWith(interactorRef.Get()))
		{
			CanceledInteractions.AddUnique(interactorRef);
		}
	}
	for (auto& interactorRef : CanceledInteractions)
	{
		FRCIconInteractionState* interState = ActiveInteractions.Find(interactorRef);
		if (interState)
		{
			URCInteractorComponent* interactor = interactorRef.Get();
			if ((interState->Starting==false) &&
				OnInteractionCanceled.IsBound() &&
				ShouldTriggerEventsFor(interactor))
			{
				OnInteractionCanceled.Broadcast(interactor);
			}
			if (interactor)
			{
				interactor->ClearCurrentInteraction();
			}
			ActiveInteractions.Remove(interactorRef);
		}
	}
	CanceledInteractions.Reset();
	for (auto& [interactorRef, interactionState] : ActiveInteractions)
	{
		if (interactionState.Starting)
		{
			interactionState.Starting = false;
			if (OnInteractionStarted.IsBound() && ShouldTriggerEventsFor(interactorRef.Get()))
			{
				OnInteractionStarted.Broadcast(interactorRef.Get());
			}
		}
		interactionState.Time += DeltaTime;
		if (interactionState.Time > InteractionTime)
		{
			CompletedInteractions.Add(interactorRef);
		}
	}
	for (auto& interactorRef : CompletedInteractions)
	{
		URCInteractorComponent* interactor = interactorRef.Get();
		if (OnInteractionCompleted.IsBound() && ShouldTriggerEventsFor(interactor))
		{
			OnInteractionCompleted.Broadcast(interactor);
		}
		if (interactor)
		{
			interactor->ClearCurrentInteraction();
		}
		ActiveInteractions.Remove(interactorRef);
	}
	CompletedInteractions.Reset();

	if (ActiveInteractions.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}
}

bool URCIconInteractionComponent::ShouldTriggerEventsFor(class URCInteractorComponent* interactor) const
{
	if (IsValid(interactor) == false)
		return false;
	if (InteractOnAuthority && (interactor->GetOwnerRole() == ENetRole::ROLE_Authority))
		return true;
	if (InteractOnAutonomousProxy && (interactor->GetOwnerRole() == ENetRole::ROLE_AutonomousProxy))
		return true;
	return false;
}