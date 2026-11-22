// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RCIconComponent.h"
#include "RCIconInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIconInteractionDlg, URCInteractorComponent*, Interactor);

USTRUCT()
struct RCGUI_API FRCIconInteractionState
{
	GENERATED_BODY()

	FRCIconInteractionState() = default;
	FRCIconInteractionState(APlayerController* controller)
	{
		Controller = controller;
	}

	bool Starting = true;
	float Time = 0.0f;

	UPROPERTY()
	TWeakObjectPtr<APlayerController> Controller = nullptr;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RCGUI_API URCIconInteractionComponent : public URCIconComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Icon")
	float InteractionRange = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Icon")
	float InteractionTime = 1.0f;

	// the view model of the interaction for the icon's widget, next to the icon's, created with the first update on the
	// local viewer's machine
	UPROPERTY(Transient)
	TObjectPtr<class URCIconInteractionViewModel> InteractionViewModel;

	UPROPERTY(EditAnywhere, Category = "Icon")
	bool InteractOnAuthority = true;

	UPROPERTY(EditAnywhere, Category = "Icon")
	bool InteractOnAutonomousProxy = false;

public:
	UPROPERTY(BlueprintAssignable)
	FIconInteractionDlg OnInteractionStarted;

	UPROPERTY(BlueprintAssignable)
	FIconInteractionDlg OnInteractionCompleted;

	UPROPERTY(BlueprintAssignable)
	FIconInteractionDlg OnInteractionCanceled;

public:
	URCIconInteractionComponent();

	UFUNCTION(BlueprintCallable)
	bool GetInteractOnAuthority() const { return InteractOnAuthority; }

	UFUNCTION(BlueprintCallable)
	bool GetInteractOnAutonomousProxy() const { return InteractOnAutonomousProxy; }

	UFUNCTION(BlueprintCallable)
	void TryToInteract(URCInteractorComponent* interactor);

	UFUNCTION(BlueprintCallable)
	void CancelInteraction(URCInteractorComponent* interactor);

	FRCIconInteractionState* GetInteractorState(APlayerController* controller);

	bool CanInteractWith(URCInteractorComponent* interactor) const;

protected:
	virtual void UpdateViewModels(class APlayerController* viewer, float distanceToViewer, bool focused) override;
	virtual void SetViewModels() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool ShouldTriggerEventsFor(class URCInteractorComponent* interactor) const;


private:
	//weak so an interactor destroyed mid-interaction is canceled on the next tick
	//instead of leaving a GC-nulled key in the map
	TMap<TWeakObjectPtr<URCInteractorComponent>, FRCIconInteractionState> ActiveInteractions;
	TArray<TWeakObjectPtr<URCInteractorComponent>> CanceledInteractions;
	TArray<TWeakObjectPtr<URCInteractorComponent>> CompletedInteractions;
};
