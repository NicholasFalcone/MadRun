// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MadRunGameMode.h"
#include "OffroadGameMode.generated.h"

/**
 *  Simple GameMode for an offroad vehicle game
 */
UCLASS(abstract)
class AOffroadGameMode : public AMadRunGameMode
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AOffroadGameMode();
};
