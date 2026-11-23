#pragma once

#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

inline bool IsUIRegressionEnvironment(const UWorld* World)
{
    if (!World || World->WorldType != EWorldType::Game
        || !FParse::Param(FCommandLine::Get(), TEXT("UIRegressionReplay")))
    {
        return false;
    }

    if (World->GetNetMode() == NM_ListenServer)
    {
        FString address;
        return FParse::Value(FCommandLine::Get(), TEXT("multihome="), address) && address == TEXT("127.0.0.1");
    }
    const auto* driver = World->GetNetDriver();
    return World->GetNetMode() == NM_Client && driver && driver->ServerConnection
        && driver->ServerConnection->LowLevelGetRemoteAddress(false) == TEXT("127.0.0.1");
}

#endif
