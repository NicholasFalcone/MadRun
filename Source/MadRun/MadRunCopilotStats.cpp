// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunCopilotStats.h"

void FCopilotStats::RecalculateDerivedStats()
{
	// Repair Speed: 50% base skill = 1.0x, 100% skill = 1.5x
	RepairSpeedMultiplier = 1.0f + (MechanicsSkill / 100.0f) * 0.5f;

	// Accuracy Bonus: 50% base skill = 0 degrees, 100% skill = -5 degrees (better)
	AccuracyBonus = (AccuracySkill / 100.0f) * 5.0f;

	// Scavenging Find Chance: 50% base skill = 0.5, 100% skill = 0.9
	ScavengingFindChance = 0.4f + (ScavengingSkill / 100.0f) * 0.5f;
}

float FCopilotStats::GetAdjustedRepairTime(float BaseRepairTime) const
{
	return BaseRepairTime / RepairSpeedMultiplier;
}

float FCopilotStats::GetAccuracyDeviation() const
{
	// Higher accuracy skill = lower deviation (better)
	// At 50% skill: 5 degrees deviation
	// At 100% skill: 0 degrees deviation
	return FMath::Max(0.0f, 10.0f - AccuracyBonus);
}

float FCopilotStats::GetLootFindChance() const
{
	return FMath::Clamp(ScavengingFindChance, 0.0f, 1.0f);
}
