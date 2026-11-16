// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MadRunCopilotStats.generated.h"

USTRUCT(BlueprintType)
struct FCopilotStats
{
	GENERATED_BODY()

	// Mechanics skill (0-100) - affects repair speed and vehicle maintenance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copilot|Stats", meta = (ClampMin = "0", ClampMax = "100"))
	float MechanicsSkill = 50.0f;

	// Accuracy skill (0-100) - affects weapon accuracy and fire rate
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copilot|Stats", meta = (ClampMin = "0", ClampMax = "100"))
	float AccuracySkill = 50.0f;

	// Scavenging skill (0-100) - affects loot find rate and resource gathering
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copilot|Stats", meta = (ClampMin = "0", ClampMax = "100"))
	float ScavengingSkill = 50.0f;

	// Derived values - calculated from base stats
	
	// Repair speed multiplier (1.0 = normal speed)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Copilot|Derived")
	float RepairSpeedMultiplier = 1.0f;

	// Weapon accuracy bonus (degrees of aim reduction)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Copilot|Derived")
	float AccuracyBonus = 0.0f;

	// Scavenging find chance (0.0 - 1.0)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Copilot|Derived")
	float ScavengingFindChance = 0.5f;

	// Called to recalculate derived values
	void RecalculateDerivedStats();

	// Get the repair time for a part given its base repair time
	float GetAdjustedRepairTime(float BaseRepairTime) const;

	// Get the accuracy deviation in degrees
	float GetAccuracyDeviation() const;

	// Get the chance to find loot (0.0 - 1.0)
	float GetLootFindChance() const;
};
