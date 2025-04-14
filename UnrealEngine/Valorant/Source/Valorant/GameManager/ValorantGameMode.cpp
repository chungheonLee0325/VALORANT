// Copyright Epic Games, Inc. All Rights Reserved.

#include "ValorantGameMode.h"
#include "Valorant/ValorantCharacter.h"
#include "UObject/ConstructorHelpers.h"

AValorantGameMode::AValorantGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character

	// KBD: 폴더링 하는 동안 불러오지 않도록 함
	// static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	// DefaultPawnClass = PlayerPawnClassFinder.Class;
}
