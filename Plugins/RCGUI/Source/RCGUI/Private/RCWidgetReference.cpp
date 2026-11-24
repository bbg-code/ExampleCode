// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#include "RCWidgetReference.h"
#include "Components/PanelWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "MVVMViewModelBase.h"
#include "View/MVVMView.h"

void FRCWidgetReference::Create(UWorld* world, UPanelWidget* parent)
{
	if (IsValid(Instance) || !WidgetClass || !IsValid(world) || world->bIsTearingDown)
		return;

	AddInstance(CreateWidget<UUserWidget>(world, WidgetClass), parent);
}

void FRCWidgetReference::Create(APlayerController* owningPlayer, UPanelWidget* parent)
{
	if (IsValid(Instance) || !WidgetClass || !IsValid(owningPlayer))
		return;
	UWorld* world = owningPlayer->GetWorld();
	if (!IsValid(world) || world->bIsTearingDown)
		return;

	AddInstance(CreateWidget<UUserWidget>(owningPlayer, WidgetClass), parent);
}

void FRCWidgetReference::Destroy()
{
	if (IsValid(Instance))
	{
		Instance->RemoveFromParent();
	}
	Instance = nullptr;
}

void FRCWidgetReference::SetViewModel(UMVVMViewModelBase* viewModel)
{
	SetViewModel(Instance, viewModel);
}

void FRCWidgetReference::SetViewModel(UUserWidget* widget, UMVVMViewModelBase* viewModel)
{
	if (!IsValid(widget) || !IsValid(viewModel))
		return;

	UMVVMView* view = widget->GetExtension<UMVVMView>();
	const bool accepted = view && view->SetViewModelByClass(TScriptInterface<INotifyFieldValueChanged>(viewModel));
}

void FRCWidgetReference::AddInstance(UUserWidget* widget, UPanelWidget* parent)
{
	if (!IsValid(widget))
		return;

	if (DesiredSizeInViewport.SizeSquared() > 0)
	{
		widget->SetDesiredSizeInViewport(DesiredSizeInViewport);
	}
	if (parent)
	{
		parent->AddChild(widget);
	}
	else
	{
		widget->AddToViewport();
	}
	Instance = widget;
}
