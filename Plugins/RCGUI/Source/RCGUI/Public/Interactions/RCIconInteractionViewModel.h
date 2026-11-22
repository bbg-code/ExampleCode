// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Components/SlateWrapperTypes.h"
#include "RCIconInteractionViewModel.generated.h"

// MVVM view model of a URCIconInteractionComponent's interaction for its widget, next to its icon's URCIconViewModel: the
// interaction's state for the local viewer, InteractionVisibility binds straight to the visibility of the widget's
// interaction part
UCLASS()
class RCGUI_API URCIconInteractionViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

	// Visible while the icon is focused and the viewer's pawn can interact with it (CanInteractWith: enabled, within
	// InteractionRange), Hidden otherwise
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Setter, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	ESlateVisibility InteractionVisibility = ESlateVisibility::Hidden;

	// 0 to 1, how far the viewer's running interaction is, 0 while none runs
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Setter, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	float InteractionProgress = 0.0f;

public:
	ESlateVisibility GetInteractionVisibility() const { return InteractionVisibility; }
	void SetInteractionVisibility(ESlateVisibility interactionVisibility) { UE_MVVM_SET_PROPERTY_VALUE(InteractionVisibility, interactionVisibility); }

	float GetInteractionProgress() const { return InteractionProgress; }
	void SetInteractionProgress(float interactionProgress) { UE_MVVM_SET_PROPERTY_VALUE(InteractionProgress, interactionProgress); }
};
