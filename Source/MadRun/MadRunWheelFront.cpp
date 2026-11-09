// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunWheelFront.h"
#include "UObject/ConstructorHelpers.h"

UMadRunWheelFront::UMadRunWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}