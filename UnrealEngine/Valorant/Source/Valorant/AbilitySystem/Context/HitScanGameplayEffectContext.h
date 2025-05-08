#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "HitScanGameplayEffectContext.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FHitScanGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_USTRUCT_BODY()

	int Damage = 0;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	virtual FGameplayEffectContext* Duplicate() const override
	{
		FHitScanGameplayEffectContext* NewContext = new FHitScanGameplayEffectContext(*this);
		return NewContext;
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override
	{
		Super::NetSerialize(Ar, Map, bOutSuccess);
		Ar << Damage;
		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FHitScanGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FHitScanGameplayEffectContext>
{
	enum { WithNetSerializer = true, WithCopy = true };
};