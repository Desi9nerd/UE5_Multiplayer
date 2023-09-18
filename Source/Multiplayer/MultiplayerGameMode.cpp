// Copyright Epic Games, Inc. All Rights Reserved.

#include "MultiplayerGameMode.h"
#include "MultiplayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMultiplayerGameMode::AMultiplayerGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/BP/Character/BP_BaseCharacter.BP_BaseCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}