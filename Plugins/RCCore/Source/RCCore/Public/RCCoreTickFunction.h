// Copyright Ergistal Collective, Bejan Bogdan Gabriel. All Rights Reserved.

#pragma once
#include <functional>
#include "CoreMinimal.h"
#include "RCCoreTickFunction.generated.h"

USTRUCT()
struct RCCORE_API FRCCoreTickFunction : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	std::function<void(float)> TickDelegate;

	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override
	{
		if (TickDelegate)
		{
			TickDelegate(DeltaTime);
		}
	}
	virtual FString DiagnosticMessage() override
	{
		return TEXT("FRCCoreTickFunction");
	}
	virtual FName DiagnosticContext(bool bDetailed) override
	{
		return FName(TEXT("FRCCoreTickFunction"));
	}
};

template<>
struct TStructOpsTypeTraits<FRCCoreTickFunction> : public TStructOpsTypeTraitsBase2<FRCCoreTickFunction>
{
	enum
	{
		WithCopy = false
	};
};