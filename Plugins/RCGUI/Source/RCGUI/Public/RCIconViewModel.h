// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Components/SlateWrapperTypes.h"
#include "RCIconViewModel.generated.h"

// MVVM view model of an icon (URCIconComponent, URCIconInteractionComponent) for its widget: the icon's state for the
// local viewer, DistanceText and DistanceVisibility bind straight to the widget's distance text; its place on screen and
// its fade the icons subsystem sets on the widget itself
UCLASS()
class RCGUI_API URCIconViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

	// the one icon closest to the screen centre
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Setter, Category = "Icon", meta = (AllowPrivateAccess = "true"))
	bool IsFocused = false;

	// the whole meters from the viewer's pawn, e.g. "12 m", see SetDistance
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Category = "Icon", meta = (AllowPrivateAccess = "true"))
	FText DistanceText;

	// Visible beyond the icon's DistanceTextRangeMin, Hidden closer
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, Setter, Category = "Icon", meta = (AllowPrivateAccess = "true"))
	ESlateVisibility DistanceVisibility = ESlateVisibility::Hidden;

	// the whole meters DistanceText shows, none before the first SetDistance
	int32 Distance = INDEX_NONE;

public:
	bool GetIsFocused() const { return IsFocused; }
	void SetIsFocused(bool isFocused) { UE_MVVM_SET_PROPERTY_VALUE(IsFocused, isFocused); }

	FText GetDistanceText() const { return DistanceText; }
	// whole meters, formats DistanceText only when they change, not on every update
	void SetDistance(int32 distance);

	ESlateVisibility GetDistanceVisibility() const { return DistanceVisibility; }
	void SetDistanceVisibility(ESlateVisibility distanceVisibility) { UE_MVVM_SET_PROPERTY_VALUE(DistanceVisibility, distanceVisibility); }
};
