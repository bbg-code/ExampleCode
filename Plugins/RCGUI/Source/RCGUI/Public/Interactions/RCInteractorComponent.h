// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "RCDoubleBufferArray.h"

#include "RCInteractorComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RCGUI_API URCInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	URCInteractorComponent();

	UFUNCTION(BlueprintCallable)
	void TryToInteract();

	UFUNCTION(BlueprintCallable)
	void CancelInteraction();

	void ClearCurrentInteraction();

	FRCDoubleBufferArray<TWeakObjectPtr<URCIconInteractionComponent>>& GetFocusedInteractionsArray()
	{
		return FocusedInteractions;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

private:
	UPROPERTY(Transient)
	class URCIconInteractionComponent* CurrentInteraction = nullptr;

	FRCDoubleBufferArray<TWeakObjectPtr<URCIconInteractionComponent>> FocusedInteractions;

	UFUNCTION(Server, Reliable)
	void RPC_C2S_StartInteraction(URCIconInteractionComponent* interaction);

	UFUNCTION(Server, Reliable)
	void RPC_C2S_CancelInteraction();

};
