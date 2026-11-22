// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#include "RCIconViewModel.h"

void URCIconViewModel::SetDistance(int32 distance)
{
	if (distance == Distance)
		return;

	Distance = distance;
	DistanceText = FText::Format(NSLOCTEXT("RCIconViewModel", "DistanceText", "{0} m"), FText::AsNumber(distance));
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DistanceText);
}
