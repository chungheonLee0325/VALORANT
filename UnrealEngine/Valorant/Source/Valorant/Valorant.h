// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * NetMode에 따라 접두사가 달라지고 이외는 모두 동일한 로그
 */
#define NET_LOG(CategoryName, Verbosity, Format, ...) \
	{ \
		const auto* World = GetWorld(); \
		FString Prefix; \
		if (World) \
		{ \
			ENetMode NetMode = World->GetNetMode(); \
			if (NetMode == NM_DedicatedServer || NetMode == NM_ListenServer) \
			{ \
				Prefix = TEXT("[SERVER] "); \
			} \
			else if (NetMode == NM_Client) \
			{ \
				Prefix = TEXT("[CLIENT] "); \
			} \
			else \
			{ \
				Prefix = TEXT("[STANDALONE] "); \
			} \
			const FString& Nickname = USubsystemSteamManager::GetDisplayName(GetWorld()); \
			Prefix += Nickname; \
		} \
		else \
		{ \
			Prefix = TEXT("[UNKNOWN] "); \
		} \
		UE_LOG(CategoryName, Verbosity, TEXT("%s: %s"), *Prefix, *FString::Printf(Format, ##__VA_ARGS__)); \
	}

#define SCREEN_LOG(Format, ...) \
	if (GEngine) \
	{ \
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, *FString::Printf(Format, ##__VA_ARGS__)); \
	}

template <typename TEnum>
static FString EnumToString(TEnum EnumValue)
{
	return StaticEnum<TEnum>()->GetNameStringByValue(static_cast<int8>(EnumValue));
}