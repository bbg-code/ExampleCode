// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.


#include "RCIconComponent.h"
#include "RCIconsSubsystem.h"
#include "RCIconViewModel.h"

URCIconComponent::URCIconComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void URCIconComponent::SetIconEnabled(bool iconEnabled)
{
	Enabled = iconEnabled;
	if (URCIconsSubsystem* guiSubsystem = GetWorld()->GetSubsystem<URCIconsSubsystem>())
	{
		guiSubsystem->SetIconEnabled(iconEnabled, IconID);
	}
}

void URCIconComponent::BeginPlay()
{
	Super::BeginPlay();
	if (URCIconsSubsystem* guiSubsystem = GetWorld()->GetSubsystem<URCIconsSubsystem>())
	{
		IconID = guiSubsystem->RegisterIcon(
			this,
			Enabled,
			VisibleRangeMin,
			VisibleRangeMax,
			VisibleOffScreen,
			this,
			FTransform::Identity);
	}
}

void URCIconComponent::EndPlay(EEndPlayReason::Type reason)
{
	Super::EndPlay(reason);
	if (URCIconsSubsystem* guiSubsystem = GetWorld()->GetSubsystem<URCIconsSubsystem>())
	{
		guiSubsystem->UnRegisterID(IconID);
	}
}

void URCIconComponent::UpdateViewModels(APlayerController* viewer, float distanceToViewer, bool focused)
{
	// on the first update, before the subsystem creates the widget, see SetViewModels
	if (!IconViewModel)
	{
		IconViewModel = NewObject<URCIconViewModel>(this);
	}
	IconViewModel->SetIsFocused(focused);
	IconViewModel->SetDistance(FMath::TruncToInt32(distanceToViewer / 100.0f));
	IconViewModel->SetDistanceVisibility(
		(distanceToViewer > DistanceTextRangeMin) ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

void URCIconComponent::OnIconCreated()
{
	// the subsystem updates every icon it can create a widget for, see UpdateViewModels
	ensure(IconViewModel);
	SetViewModels();
	DlgOnIconCreated.Broadcast();
}

void URCIconComponent::OnIconDestroyed()
{
	DlgOnIconDestroyed.Broadcast();
}

void URCIconComponent::SetViewModels()
{
	IconWidget.SetViewModel(IconViewModel);
}
