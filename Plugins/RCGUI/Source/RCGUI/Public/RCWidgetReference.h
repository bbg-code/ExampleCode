// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RCWidgetReference.generated.h"

class APlayerController;
class UMVVMViewModelBase;
class UPanelWidget;
class UWorld;

// a widget created from editor data: its class, the size it asks for in the viewport, and the created widget
USTRUCT()
struct RCGUI_API FRCWidgetReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Widget")
	TSubclassOf<UUserWidget> WidgetClass;

	// set on the created widget when above zero (SetDesiredSizeInViewport), for a widget added to the viewport
	UPROPERTY(EditAnywhere, Category = "Widget")
	FVector2D DesiredSizeInViewport{ 0.0f };

	// creates the widget unless it exists, into the viewport without a parent, into the parent panel otherwise
	void Create(UWorld* world, UPanelWidget* parent);
	// the same, owned by a player, e.g. a local player's own HUD in split screen
	void Create(APlayerController* owningPlayer, UPanelWidget* parent);
	void Destroy();

	// sets the view model on the created widget, into its view model of the same class (creation type Manual in the
	// widget Blueprint); nothing when the widget has none of that class
	void SetViewModel(UMVVMViewModelBase* viewModel);
	// the same on any widget, e.g. a child of a created widget
	static void SetViewModel(UUserWidget* widget, UMVVMViewModelBase* viewModel);

	// true from Create until Destroy
	bool IsInstanceValid() const { return IsValid(Instance); }
	// the created widget, null before Create and after Destroy
	UUserWidget* GetInstance() const { return Instance; }

private:
	// takes the widget just created: its size, its parent, kept as the instance
	void AddInstance(UUserWidget* widget, UPanelWidget* parent);

	// the created widget, held from Create until Destroy: removed from its parent by anything else, it stays alive here
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Instance;
};
