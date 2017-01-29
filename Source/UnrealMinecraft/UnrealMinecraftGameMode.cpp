// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealMinecraft.h"
#include "UnrealMinecraftGameMode.h"
#include "UnrealMinecraftHUD.h"
#include "UnrealMinecraftCharacter.h"

AUnrealMinecraftGameMode::AUnrealMinecraftGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AUnrealMinecraftHUD::StaticClass();
}
