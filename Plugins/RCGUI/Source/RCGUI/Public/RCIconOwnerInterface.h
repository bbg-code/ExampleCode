// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RCIconOwnerInterface.generated.h"

struct FRCWidgetReference;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class URCIconOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

// an icon registered with the URCIconsSubsystem, on the local viewer's machine only: the owner's view models carry the
// icon's state, the subsystem creates, lays out and destroys its widget
class RCGUI_API IRCIconOwnerInterface
{
	GENERATED_BODY()

public:
	// the icon's widget, created by the subsystem while the icon is visible to the viewer
	virtual FRCWidgetReference& GetIconWidget() = 0;
	// every frame while the icon is in the viewer's range or fading out, first before its widget is created: creates the
	// view models on the first call, fills them; the viewer is the first local player's controller, never null
	virtual void UpdateViewModels(class APlayerController* viewer, float distanceToViewer, bool focused) = 0;
	// right after the subsystem created the widget, the icon comes into view: sets the view models on it, created and
	// filled by UpdateViewModels
	virtual void OnIconCreated() = 0;
	// right before the subsystem destroys the widget: the icon faded out, or it is unregistered
	virtual void OnIconDestroyed() = 0;
};
