// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include <tuple>
#include <type_traits>
#include <utility>

//key -> index map over a dense array of (key, data) pairs, iterated through GetArray()
//Remove swaps the last element into the removed slot, so element order and indices change on every remove
//pointers from Find, references from FindOrAdd and views from GetArray are invalidated by any add (the array
//can reallocate) or remove (elements are swapped); an index from FindOrAddIdx survives adds but not a remove
template<class K, class D>
struct FRCMappedDataArray
{
	using KeyType = K;
	using PairType = std::pair<KeyType const, D>;
	//this container is not a UPROPERTY, so a pointer stored in it is neither owned nor seen by GC: the object can be
	//destroyed or collected under it. TObjectPtr has the same problem but can't be rejected here without a CoreUObject
	//include, so don't store it either; use values, TSharedPtr or TWeakObjectPtr
	static_assert(std::is_pointer<D>::value==false, "Array data type must not be of pointer type, the container neither owns it nor reports it to GC");

	bool Contains(KeyType key) const
	{
		return IdxMap.Find(key) != nullptr;
	}

	//FindOrAdd/FindOrAddIdx args are only read after the array may have grown, so they must not refer to data stored in this
	//container (e.g. FindOrAdd(k2, *Find(k1)) reads freed memory when the array reallocates)
	template<typename... Args>
	D& FindOrAdd(KeyType key, Args&&... args)
	{
		return FindOrAddIdx(key, std::forward<Args>(args)...).second;
	}

	template<typename... Args>
	std::pair<const int32, D&> FindOrAddIdx(KeyType key, Args&&... args)
	{
		int32& idx = IdxMap.FindOrAdd(key, -1);
		if (idx == -1)
		{
			idx = DataArray.Num();
			DataArray.Emplace(std::piecewise_construct,
				std::forward_as_tuple(key),
				std::forward_as_tuple(std::forward<Args>(args)...));
		}
		return { idx, DataArray[idx].second };
	}

	const D* Find(KeyType key) const
	{
		const int32* idx = IdxMap.Find(key);
		if (idx == nullptr)
			return nullptr;
		return &DataArray[*idx].second;
	}

	D* Find(KeyType key)
	{
		return const_cast<D*>(static_cast<const FRCMappedDataArray*>(this)->Find(key));
	}

	//key is taken by value on purpose: RemoveIdx passes a key stored in DataArray, and a reference
	//to it would read the last key once RemoveAtSwap has moved it into the removed slot
	void Remove(KeyType key)
	{
		if (int32* idx0 = IdxMap.Find(key))
		{
			//RemoveAtSwap destroys the slot while the map still points at it, so the value is
			//moved out and destroyed at the end of this block, once the map and array agree again
			D removedData = MoveTemp(DataArray[*idx0].second);

			if (int32* idx1 = IdxMap.Find(DataArray.Last().first))
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
			check(DataArray[idx].first == k);
		}
#endif
	}

	void RemoveIdx(int32 idx)
	{
		if (ensure(DataArray.IsValidIndex(idx)) == false)
			return;
		Remove(DataArray[idx].first);
	}

	TArrayView<PairType> GetArray()
	{
		return MakeArrayView(DataArray);
	}

	TArrayView<const PairType> GetArray() const
	{
		return MakeArrayView(DataArray);
	}

private:
	TMap<KeyType, int32> IdxMap;
	TArray<PairType> DataArray;
};