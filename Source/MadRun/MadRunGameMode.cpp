// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunGameMode.h"
#include "MadRunPlayerController.h"

AMadRunGameMode::AMadRunGameMode()
{
	PlayerControllerClass = AMadRunPlayerController::StaticClass();
}
