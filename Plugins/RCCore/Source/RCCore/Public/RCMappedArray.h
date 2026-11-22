// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

//key -> index map over a dense array of keys, iterated through GetArray()
//Remove swaps the last key into the removed slot, so key order and indices change on every remove
//views from GetArray are invalidated by any add (the array can reallocate) or remove (keys are swapped);
//an index from AddIdx survives adds but not a remove
template<class T>
struct FRCMappedArray
{
	using KeyType = T;

	bool Contains(KeyType key) const
	{
		return IdxMap.Find(key) != nullptr;
	}

	void Add(KeyType key)
	{
		AddIdx(key);
	}

	int32 AddIdx(KeyType key)
	{
		int32& idx = IdxMap.FindOrAdd(key, -1);
		if (idx == -1)
		{
			idx = DataArray.Num();
			DataArray.Emplace(key);
		}
		return idx;
	}

	//key is taken by value on purpose: RemoveIdx passes a key stored in DataArray, and a reference
	//to it would read the last key once RemoveAtSwap has moved it into the removed slot
	void Remove(KeyType key)
	{
		if (int32* idx0 = IdxMap.Find(key))
		{
			if (int32* idx1 = IdxMap.Find(DataArray.Last()))
			{
				*idx1 = *idx0;
			}
			DataArray.RemoveAtSwap(*idx0, 1, EAllowShrinking::No);
			IdxMap.Remove(key);
		}

		check(IdxMap.Num() == DataArray.Num());
#if DO_GUARD_SLOW
		//O(n), only in Debug builds so Remove stays O(1) elsewhere
		for (auto&[k, idx] : IdxMap)
		{
			check(DataArray[idx] == k);
		}
#endif
	}

	void RemoveIdx(int32 idx)
	{
		if (ensure(DataArray.IsValidIndex(idx)) == false)
			return;
		Remove(DataArray[idx]);
	}

	TArrayView<KeyType const> GetArray() const
	{
		return MakeArrayView(DataArray);
	}

private:                         
	TMap<KeyType, int32> IdxMap;
	TArray<KeyType> DataArray;
};