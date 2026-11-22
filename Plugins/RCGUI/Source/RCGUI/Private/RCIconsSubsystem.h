// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RCCoreTickFunction.h"
#include "RCMappedDataArray.h"
#include "RCDoubleBufferArray.h"

#include "RCIconOwnerInterface.h"
#include "Interactions/RCInteractorComponent.h"
#include "UObject/WeakInterfacePtr.h"
#include "Components/SceneComponent.h"

#include "Subsystems/WorldSubsystem.h"
#include "RCIconsSubsystem.generated.h"


UCLASS(Config = Game)
class URCIconsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// the owner's widget is created, laid out and destroyed here, its view models are updated through it
	uint32 RegisterIcon(
		IRCIconOwnerInterface* owner,
		bool enabled,
		float minRange,
		float maxRange,
		bool visibleOffScreen,
		USceneComponent* attachmentParent,
		const FTransform& relativeTransform);

	void UnRegisterID(uint32 id);

	void SetIconEnabled(bool enabled, uint32 id);

	//icons are only shown to, and focused by, the first local player
	//null on a dedicated server
	APlayerController* GetViewerController() const;

protected:
	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	virtual void OnWorldBeginPlay(UWorld& inWorld) override; //from UWorldSubsystem
	virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;  //from UWorldSubsystem
	virtual void Deinitialize() override; //from USubsystem

	void UpdateTick(float deltaTime);
	void PreSlateUpdateTick(float deltaTime);

private:
	//icons whose transform and range are updated per frame, round-robin, so the cost doesn't grow with the icon count;
	//set in DefaultGame.ini under [/Script/RCGUI.RCIconsSubsystem]
	UPROPERTY(Config, meta = (ClampMin = "1"))
	int32 IconUpdatesPerFrame = 50;

	FRCCoreTickFunction UpdateTickFunction;
	FDelegateHandle PreSlateUpdateDlgHandle;

	struct RCIconData
	{
		bool Enabled{ true };
		bool IsActive{ false };
		TWeakInterfacePtr<IRCIconOwnerInterface> Owner;
		float                           MinRange{ 0 };
		float                           MaxRange{ 0 };
		bool                            VisibleOffScreen{ true };

		TWeakObjectPtr<USceneComponent> AttachmentParent{ nullptr };
		FTransform                      RelativeTransform{ FTransform::Identity };

		FTransform                      WorldTransform{ FTransform::Identity };

		FRCDoubleBufferArray<TWeakObjectPtr<URCInteractorComponent>> InteractorsInRange;

		float                           SSWidgetFadeTarget{ 0.0f };
		float                           SSWidgetFade{ 0.0f };

		float                           Distance{ 0.0f };
		FVector2D                       ScreenPosition{ 0.0 };
		bool                            IsFocused{ false };
	};

	FRCMappedDataArray<uint32, TSharedPtr<RCIconData>> Icons;
	FRCDoubleBufferArray<TSharedPtr<RCIconData>> ActiveIcons;

	uint32 NextID = 0;
	int32 UpdateIdx = 0;
};
