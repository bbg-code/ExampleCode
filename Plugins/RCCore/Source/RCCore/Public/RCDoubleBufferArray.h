// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

//Add writes one buffer while GetArray reads the one filled before the last Flip; Add never touches
//the read buffer, so it doesn't invalidate a view from GetArray
//Flip resets the buffer that becomes the write buffer, so a view from GetArray must not be kept across
//a Flip: the elements it points at are destroyed while the view keeps its old Num
template<class T>
struct FRCDoubleBufferArray
{
	using ElementType = T;

	//by value then moved: one copy for an lvalue, none for a temporary
	void Add(ElementType element)
	{
		if (IsFlipped)
		{
			DataArrayA.Emplace(MoveTemp(element));
		}
		else
		{
			DataArrayB.Emplace(MoveTemp(element));
		}
	}

	void Flip()
	{
		IsFlipped = !IsFlipped;
		if (IsFlipped)
		{
			DataArrayA.Reset();
		}
		else
		{
			DataArrayB.Reset();
		}
	}

	TArrayView<ElementType const> GetArray() const
	{
		return 
			IsFlipped ? MakeArrayView(DataArrayB) : MakeArrayView(DataArrayA);
	}

private:
	TArray<ElementType> DataArrayA;
	TArray<ElementType> DataArrayB;
	bool IsFlipped = false;
};