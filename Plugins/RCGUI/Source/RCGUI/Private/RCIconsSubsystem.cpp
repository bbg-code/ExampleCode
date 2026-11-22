// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.


#include "RCIconsSubsystem.h"
#include "Interactions/RCIconInteractionComponent.h"
#include "RCWidgetReference.h"
#include "Interactions/RCInteractorComponent.h"
#include <Blueprint/WidgetLayoutLibrary.h>
#include "Engine/Engine.h"

namespace
{
	//moves point along the ray from center onto the border of the rect (center +- halfSize)
	//if it lies outside the rect, or always when forceToEdge is set
	FVector2D clampToRectEdge(const FVector2D& center, const FVector2D& halfSize, const FVector2D& point, bool forceToEdge)
	{
		const FVector2D dir = point - center;
		if (dir.IsNearlyZero())
		{
			//directly behind the viewer, park it at the bottom edge
			return forceToEdge ? center + FVector2D(0.0, halfSize.Y) : point;
		}
		const double scaleX = FMath::IsNearlyZero(dir.X) ? DBL_MAX : halfSize.X / FMath::Abs(dir.X);
		const double scaleY = FMath::IsNearlyZero(dir.Y) ? DBL_MAX : halfSize.Y / FMath::Abs(dir.Y);
		const double scale = FMath::Min(scaleX, scaleY);
		return (forceToEdge || scale < 1.0) ? center + dir * scale : point;
	}
}

uint32 URCIconsSubsystem::RegisterIcon(
	IRCIconOwnerInterface* owner,
	bool enabled,
	float minRange,
	float maxRange,
	bool visibleOffScreen,
	USceneComponent* attachmentParent,
	const FTransform& relativeTransform)
{
	if (ensure(owner) == false)
		return 0;
	if (ensure(IsValid(owner->GetIconWidget().WidgetClass.Get())) == false)
		return 0;

	while (Icons.Contains(++NextID) || (NextID == 0));

	auto& data = Icons.FindOrAdd(NextID, MakeShared<RCIconData>());
	auto* iconData = data.Get();

	iconData->Owner = TWeakInterfacePtr<IRCIconOwnerInterface>(Cast<UObject>(owner));

	iconData->Enabled = enabled;
	iconData->MinRange = minRange;
	iconData->MaxRange = maxRange;
	iconData->VisibleOffScreen = visibleOffScreen;

	iconData->AttachmentParent = TWeakObjectPtr<USceneComponent>(attachmentParent);
	iconData->RelativeTransform = relativeTransform;

	if (iconData->AttachmentParent.IsValid())
	{
		iconData->WorldTransform =
			iconData->AttachmentParent.Get()->GetComponentTransform() * iconData->RelativeTransform;
	}
	else
	{
		iconData->WorldTransform = relativeTransform;
	}

	return NextID;
}

void URCIconsSubsystem::UnRegisterID(uint32 id)
{
	if (auto* iconIrf = Icons.Find(id))
	{
		auto* iconData = iconIrf->Get();
		IRCIconOwnerInterface* owner = iconData->Owner.Get();
		if (owner && owner->GetIconWidget().IsInstanceValid())
		{
			owner->OnIconDestroyed();
			owner->GetIconWidget().Destroy();
		}
		//ActiveIcons can hold the data until the end of its pass
		iconData->Owner.Reset();
	}
	Icons.Remove(id);
}

void URCIconsSubsystem::SetIconEnabled(bool enabled, uint32 id)
{
	if (auto* iconIrf = Icons.Find(id))
	{
		auto* iconData = iconIrf->Get();
		iconData->Enabled = enabled;
	}
}

void URCIconsSubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
	Super::OnWorldBeginPlay(inWorld);

	UpdateTickFunction.bCanEverTick = true;
	UpdateTickFunction.bStartWithTickEnabled = true;
	UpdateTickFunction.bRunOnAnyThread = false;
	UpdateTickFunction.bAllowTickOnDedicatedServer = true;
	UpdateTickFunction.TickGroup = ETickingGroup::TG_DuringPhysics;
	UpdateTickFunction.TickInterval = 0.0f;
	UpdateTickFunction.TickDelegate = [this](float deltaTime)
		{
			UpdateTick(deltaTime);
		};
	UpdateTickFunction.RegisterTickFunction(inWorld.PersistentLevel);
	UpdateTickFunction.SetTickFunctionEnable(true);

	//slate is not initialized on a dedicated server
	if (FSlateApplication::IsInitialized())
	{
		PreSlateUpdateDlgHandle = FSlateApplication::Get().OnPreTick().AddLambda([this](float deltaTime)
			{
				PreSlateUpdateTick(deltaTime);
			});
	}
}

bool URCIconsSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
	return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}

void URCIconsSubsystem::Deinitialize()
{
	Super::Deinitialize();

	UpdateTickFunction.UnRegisterTickFunction();
	if (PreSlateUpdateDlgHandle.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnPreTick().Remove(PreSlateUpdateDlgHandle);
	}
}

APlayerController* URCIconsSubsystem::GetViewerController() const
{
	return GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
}

void URCIconsSubsystem::UpdateTick(float deltaTime)
{
	APlayerController* viewer = GetViewerController();
	if (viewer == nullptr)
		return;
	APawn* viewerPawn = viewer->GetPawn();
	URCInteractorComponent* viewerInteractor =
		viewerPawn ? viewerPawn->GetComponentByClass<URCInteractorComponent>() : nullptr;

	auto iconsArray = Icons.GetArray();
	//icons may have been unregistered since the last update, nothing is left to visit in this pass
	if (UpdateIdx >= iconsArray.Num())
	{
		UpdateIdx = 0;
		ActiveIcons.Flip();
	}
	for (int updateCount = 0; updateCount < (FMath::Min(IconUpdatesPerFrame, iconsArray.Num())); updateCount++)
	{
		auto&[id, iconRef] = iconsArray[UpdateIdx];
		auto* iconData = iconRef.Get();

		if (iconData->AttachmentParent.IsValid())
		{
			iconData->WorldTransform =
				iconData->AttachmentParent.Get()->GetComponentTransform() * iconData->RelativeTransform;
		}

		iconData->IsActive = false;
		iconData->InteractorsInRange.Flip();

		//without a pawn nothing is in range, so visible widgets fade out instead of freezing
		bool isInRange = false;
		if (viewerPawn)
		{
			double distanceToPawnSQ =
				(iconData->WorldTransform.GetLocation() - viewerPawn->GetActorLocation()).SizeSquared();
			iconData->Distance = FMath::Sqrt(distanceToPawnSQ);

			isInRange =
				iconData->Enabled &&
				(distanceToPawnSQ > FMath::Square(iconData->MinRange)) &&
				(distanceToPawnSQ < FMath::Square(iconData->MaxRange));
		}

		if (isInRange && viewerInteractor)
		{
			iconData->InteractorsInRange.Add(viewerInteractor);
		}

		IRCIconOwnerInterface* owner = iconData->Owner.Get();
		bool newVisible = (owner && owner->GetIconWidget().IsInstanceValid()) || isInRange;

		if (newVisible)
		{
			iconData->IsActive = true;
			iconData->SSWidgetFadeTarget = (isInRange ? 1.0f : 0.0f);
			ActiveIcons.Add(iconRef);
		}

		//the pass ends after its last icon is added, so ActiveIcons lists every icon of the pass once
		UpdateIdx++;
		if (UpdateIdx >= iconsArray.Num())
		{
			UpdateIdx = 0;
			ActiveIcons.Flip();
		}
	}

	if (viewerInteractor)
	{
		viewerInteractor->GetFocusedInteractionsArray().Flip();
	}

	//before PreSlateUpdateTick on the same icons, so the view models exist and are filled when it creates a widget
	for (auto& icon : ActiveIcons.GetArray())
	{
		auto* iconData = icon.Get();

		IRCIconOwnerInterface* owner = iconData->Owner.Get();
		if (owner == nullptr)
			continue;

		if (iconData->IsFocused)
		{
			if (URCIconInteractionComponent* interaction =
				Cast<URCIconInteractionComponent>(iconData->Owner.GetObject()))
			{
				for (auto& interactorRef : iconData->InteractorsInRange.GetArray())
				{
					if (interaction->CanInteractWith(interactorRef.Get()))
					{
						interactorRef->GetFocusedInteractionsArray().Add(interaction);
					}
				}
			}
		}

		owner->UpdateViewModels(viewer, iconData->Distance, iconData->IsFocused);

		iconData->IsFocused = false;
	}
}

void URCIconsSubsystem::PreSlateUpdateTick(float deltaTime)
{
	//the view is the same for every icon, resolve it once per frame
	APlayerController* controller = GetViewerController();
	if (!IsValid(controller))
		return;

	ULocalPlayer* localPlayer = controller->GetLocalPlayer();
	if (!IsValid(localPlayer))
		return;

	if (!IsValid(localPlayer->ViewportClient))
		return;

	FSceneViewProjectionData projectionData;
	if (!localPlayer->GetProjectionData(localPlayer->ViewportClient->Viewport, /*out*/ projectionData))
		return;

	FVector ctrlLocation;
	FRotator ctrlRotation;
	controller->GetPlayerViewPoint(ctrlLocation, ctrlRotation);
	const FQuat ctrlQuat = ctrlRotation.Quaternion();

	FMatrix const viewProjectionMatrix = projectionData.ComputeViewProjectionMatrix();

	const FIntRect viewRect = projectionData.GetViewRect();
	FInt32Point center;
	FInt32Point extent;
	viewRect.GetCenterAndExtents(center, extent);
	const FVector2D screenCenter(center.X, center.Y);

	const float maxDistFromOrgToFocus = extent.Size() / 3.0f;

	TSharedPtr<RCIconData> focusedIcon;
	for (auto& icon : ActiveIcons.GetArray())
	{
		auto* iconData = icon.Get();
		IRCIconOwnerInterface* owner = iconData->Owner.Get();
		if (owner == nullptr)
			continue;
		FRCWidgetReference& widget = owner->GetIconWidget();

		iconData->SSWidgetFade = FMath::FInterpConstantTo(
			iconData->SSWidgetFade,
			iconData->SSWidgetFadeTarget,
			deltaTime,
			2.0f);

		if (iconData->SSWidgetFade == 0.0f)
		{
			if (widget.IsInstanceValid())
			{
				owner->OnIconDestroyed();
				widget.Destroy();
			}
		}
		else
		if (GetWorld()->GetNetMode()!=ENetMode::NM_DedicatedServer)
		{
			if (widget.IsInstanceValid()==false)
			{
				widget.Create(GetWorld(), nullptr);
				owner->OnIconCreated();
			}
		}
		
		if (UUserWidget* w = widget.GetInstance())
		{
			w->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, iconData->SSWidgetFade));

			const float scale = UWidgetLayoutLibrary::GetViewportScale(w);
			FVector2D absoluteSize = widget.DesiredSizeInViewport * scale;

			FVector iconWL = iconData->WorldTransform.GetLocation();
			FVector dirOrgToIconWL = (iconWL - projectionData.ViewOrigin).GetSafeNormal();
			float dotViewAndIconDir = (ctrlQuat.GetAxisX()).Dot(dirOrgToIconWL);

			bool isOffScreen = false;

			if (dotViewAndIconDir < 0)
			{
				iconWL -= projectionData.ViewOrigin;
				iconWL = iconWL.MirrorByVector(-ctrlQuat.GetAxisX());
				iconWL += projectionData.ViewOrigin;
				isOffScreen = true;
			}

			FVector2D newScreenPosition;
			isOffScreen |= FSceneView::ProjectWorldToScreen(
				iconWL,
				projectionData.GetConstrainedViewRect(),
				viewProjectionMatrix, newScreenPosition) == false;

			const FVector2D clipPadding = iconData->VisibleOffScreen ? absoluteSize : -absoluteSize;

			//off screen icons are pinned to the edge, on screen ones only clamped when crossing it
			const FVector2D halfSize = (FVector2D(viewRect.Width(), viewRect.Height()) - clipPadding) * 0.5;
			newScreenPosition = clampToRectEdge(
				screenCenter,
				halfSize,
				newScreenPosition,
				isOffScreen);

			FVector2D finalPos = newScreenPosition - absoluteSize / 2.0f;
			iconData->ScreenPosition = newScreenPosition;

			w->SetPositionInViewport(finalPos, true);

			float distToThis =
				(iconData->ScreenPosition - screenCenter).SizeSquared();
			if ((isOffScreen == false) && (distToThis<FMath::Square(maxDistFromOrgToFocus)))
			{
				if (focusedIcon.IsValid())
				{
					float distToFocused =
						(focusedIcon.Get()->ScreenPosition - screenCenter).SizeSquared();
					if (distToThis < distToFocused)
					{
						focusedIcon = icon;
					}
				}
				else
				{
					focusedIcon = icon;
				}
			}
		}
	}

	//always false on a dedicated server
	if (focusedIcon.IsValid())
	{
		focusedIcon->IsFocused = true;
	}
}