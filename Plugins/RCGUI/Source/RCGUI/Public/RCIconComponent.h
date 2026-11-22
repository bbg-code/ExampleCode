// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RCWidgetReference.h"
#include "RCIconOwnerInterface.h"

#include "Components/SceneComponent.h"
#include "RCIconComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRCIconEvent);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RCGUI_API URCIconComponent : public USceneComponent, public IRCIconOwnerInterface
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Icon")
	float  VisibleRangeMin = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Icon")
	float  VisibleRangeMax = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Icon")
	bool VisibleOffScreen = true;

	// cm from the viewer's pawn beyond which the widget's distance text shows (DistanceVisibility)
	UPROPERTY(EditAnywhere, Category = "Icon")
	float DistanceTextRangeMin = 1000.0f;

	// created, laid out and destroyed by the icons subsystem on the local viewer's machine, gets the view models in
	// SetViewModels
	UPROPERTY(EditAnywhere, Category = "Icon")
	FRCWidgetReference IconWidget;

	// the view model of the icon for its widget, created with the first update on the local viewer's machine
	UPROPERTY(Transient)
	TObjectPtr<class URCIconViewModel> IconViewModel;

	uint32 IconID{ 0 };

public:	
	URCIconComponent();

	UFUNCTION(BlueprintCallable)
	void SetIconEnabled(bool iconEnabled);

	// on the local viewer's machine, after the view models are set on the new widget: the icon comes into view
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "On Icon Created"))
	FRCIconEvent DlgOnIconCreated;

	// on the local viewer's machine, before the widget is destroyed: the icon faded out, or it is unregistered
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "On Icon Destroyed"))
	FRCIconEvent DlgOnIconDestroyed;

protected:
	UPROPERTY(EditAnywhere, Category = "Icon")
	bool Enabled{ true };

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type reason) override;

	virtual FRCWidgetReference& GetIconWidget() override { return IconWidget; }
	virtual void UpdateViewModels(class APlayerController* viewer, float distanceToViewer, bool focused) override;
	// sets the view models on the new widget (SetViewModels), then broadcasts DlgOnIconCreated
	virtual void OnIconCreated() override;
	virtual void OnIconDestroyed() override;
	// from OnIconCreated, each class sets its own view models
	virtual void SetViewModels();
};
