// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#include "Interactions/RCInteractorComponent.h"
#include "Interactions/RCIconInteractionComponent.h"

URCInteractorComponent::URCInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URCInteractorComponent::TryToInteract()
{
	if (ensure(GetOwnerRole() != ENetRole::ROLE_SimulatedProxy) == false)
	{
		return;
	}
	if (IsValid(CurrentInteraction))
	{
		return;
	}

	auto focusedInteractions = FocusedInteractions.GetArray();
	if (focusedInteractions.Num())
	{
		if (URCIconInteractionComponent* ic = focusedInteractions[0].Get())
		{
			CurrentInteraction =ic;
			CurrentInteraction->TryToInteract(this);
			if (CurrentInteraction->GetInteractOnAuthority() && (GetOwnerRole() != ENetRole::ROLE_Authority))
			{
				RPC_C2S_StartInteraction(CurrentInteraction);
			}
		}
	}
}

void URCInteractorComponent::CancelInteraction()
{
	if (IsValid(CurrentInteraction))
	{
		CurrentInteraction->CancelInteraction(this);
		if (CurrentInteraction->GetInteractOnAuthority() && (GetOwnerRole() != ENetRole::ROLE_Authority))
		{
			RPC_C2S_CancelInteraction();
		}
		CurrentInteraction = nullptr;
	}
}

void URCInteractorComponent::ClearCurrentInteraction()
{
	/*
	UE_LOG(LogTemp, Warning, TEXT("%i URCInteractorComponent::ClearCurrentInteraction"),
		GetOwnerRole());
	//*/

	CurrentInteraction = nullptr;
}

void URCInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URCInteractorComponent::EndPlay(EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);
}

void URCInteractorComponent::RPC_C2S_StartInteraction_Implementation(URCIconInteractionComponent* interaction)
{
	if (IsValid(CurrentInteraction))
		return;
	if (!IsValid(interaction))
		return;

	CurrentInteraction = interaction;
	CurrentInteraction->TryToInteract(this);
}

void URCInteractorComponent::RPC_C2S_CancelInteraction_Implementation()
{
	if (!IsValid(CurrentInteraction))
		return;
	CurrentInteraction->CancelInteraction(this);
	CurrentInteraction = nullptr;
}