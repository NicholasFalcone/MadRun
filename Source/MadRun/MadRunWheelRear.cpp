// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunWheelRear.h"
#include "UObject/ConstructorHelpers.h"

UMadRunWheelRear::UMadRunWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}