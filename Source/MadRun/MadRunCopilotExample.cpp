// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunCopilotExample.h"
#include "MadRunCopilot.h"
#include "MadRunCopilotStats.h"
#include "MadRunCopilotRepairComponent.h"
#include "MadRunCopilotCombatComponent.h"
#include "MadRunCopilotScavengingComponent.h"

AMadRunCopilotExample::AMadRunCopilotExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMadRunCopilotExample::BeginPlay()
{
	Super::BeginPlay();

	// Optionally spawn copilot automatically on game start
	// SpawnAndInitializeCopilot();
}

void AMadRunCopilotExample::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMadRunCopilotExample::SpawnAndInitializeCopilot()
{
	if (!GetWorld())
	{
		return;
	}

	// Spawn copilot at a fixed location (you can customize this)
	FVector SpawnLocation = GetActorLocation() + FVector(500.0f, 0.0f, 100.0f);
	FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	ExampleCopilot = GetWorld()->SpawnActor<ACopilot>(SpawnLocation, SpawnRotation, SpawnParams);

	if (ExampleCopilot)
	{
		// Set copilot stats to demonstrate different skill levels
		FCopilotStats Stats;
		Stats.MechanicsSkill = 80.0f;    // Good at repairs
		Stats.AccuracySkill = 75.0f;     // Good at shooting
		Stats.ScavengingSkill = 65.0f;   // Decent at scavenging
		ExampleCopilot->SetCopilotStats(Stats);

		LogCopilotStats();

		UE_LOG(LogTemp, Log, TEXT("AMadRunCopilotExample::SpawnAndInitializeCopilot - Copilot spawned at %s"), 
			*SpawnLocation.ToString());
	}
}

void AMadRunCopilotExample::DemonstrateRepair()
{
	if (!ExampleCopilot || !ExampleCopilot->RepairComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMadRunCopilotExample::DemonstrateRepair - No copilot or repair component"));
		return;
	}

	// For this example, we'll repair the copilot actor itself
	// In a real scenario, you would repair a specific vehicle part
	AActor* TargetToRepair = ExampleCopilot;

	// Set up event binding to log when repair completes
	ExampleCopilot->RepairComponent->OnRepairStarted.AddDynamic(this, &AMadRunCopilotExample::OnRepairStarted);
	ExampleCopilot->RepairComponent->OnRepairCompleted.AddDynamic(this, &AMadRunCopilotExample::OnRepairCompleted);

	// Start repair with a 10-second base time
	ExampleCopilot->RepairComponent->StartRepair(TargetToRepair, 10.0f, 100.0f);

	UE_LOG(LogTemp, Log, TEXT("AMadRunCopilotExample::DemonstrateRepair - Started repair on %s"), 
		*TargetToRepair->GetName());
}

void AMadRunCopilotExample::OnRepairStarted(AActor* Target, float RepairDuration)
{
	UE_LOG(LogTemp, Log, TEXT("Repair started on %s with adjusted duration: %.2f seconds"), 
		*Target->GetName(), RepairDuration);
}

void AMadRunCopilotExample::OnRepairCompleted(AActor* Target, bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("Repair completed on %s - Success: %d"), 
		*Target->GetName(), bSuccess);
}

void AMadRunCopilotExample::DemonstrateCombat()
{
	if (!ExampleCopilot || !ExampleCopilot->CombatComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMadRunCopilotExample::DemonstrateCombat - No copilot or combat component"));
		return;
	}

	// Set up weapon with 30 ammo
	ExampleCopilot->CombatComponent->SetWeapon(ExampleCopilot, 30);

	// Set up event binding
	ExampleCopilot->CombatComponent->OnFireStarted.AddDynamic(this, &AMadRunCopilotExample::OnFireStarted);
	ExampleCopilot->CombatComponent->OnFireCompleted.AddDynamic(this, &AMadRunCopilotExample::OnFireCompleted);

	// Aim and fire at a target
	AActor* Target = ExampleCopilot;  // In real scenario, this would be an enemy
	ExampleCopilot->CombatComponent->AimAtTarget(Target);
	ExampleCopilot->CombatComponent->FireAtTarget();

	UE_LOG(LogTemp, Log, TEXT("AMadRunCopilotExample::DemonstrateCombat - Accuracy deviation: %.2f degrees"), 
		ExampleCopilot->CombatComponent->GetCurrentAccuracyDeviation());
}

void AMadRunCopilotExample::OnFireStarted(AActor* Target, float Damage)
{
	UE_LOG(LogTemp, Log, TEXT("Fire started on %s with damage: %.2f"), 
		*Target->GetName(), Damage);
}

void AMadRunCopilotExample::OnFireCompleted(bool bHit)
{
	UE_LOG(LogTemp, Log, TEXT("Fire completed - Hit: %d"), bHit);
}

void AMadRunCopilotExample::DemonstrateScavenging()
{
	if (!ExampleCopilot || !ExampleCopilot->ScavengingComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMadRunCopilotExample::DemonstrateScavenging - No copilot or scavenging component"));
		return;
	}

	// Create a loot table
	TArray<FLootTable> LootTable;

	// Fuel loot
	FLootTable FuelLoot;
	FuelLoot.LootType = TEXT("Fuel");
	FuelLoot.DropChance = 60.0f;
	FuelLoot.MinAmount = 20;
	FuelLoot.MaxAmount = 50;
	LootTable.Add(FuelLoot);

	// Spare parts loot
	FLootTable PartLoot;
	PartLoot.LootType = TEXT("SpareParts");
	PartLoot.DropChance = 40.0f;
	PartLoot.MinAmount = 5;
	PartLoot.MaxAmount = 15;
	LootTable.Add(PartLoot);

	// Ammunition loot
	FLootTable AmmoLoot;
	AmmoLoot.LootType = TEXT("Ammunition");
	AmmoLoot.DropChance = 50.0f;
	AmmoLoot.MinAmount = 30;
	AmmoLoot.MaxAmount = 100;
	LootTable.Add(AmmoLoot);

	// Set up event binding
	ExampleCopilot->ScavengingComponent->OnScavengeStarted.AddDynamic(this, &AMadRunCopilotExample::OnScavengeStarted);
	ExampleCopilot->ScavengingComponent->OnScavengeCompleted.AddDynamic(this, &AMadRunCopilotExample::OnScavengeCompleted);
	ExampleCopilot->ScavengingComponent->OnLootFound.AddDynamic(this, &AMadRunCopilotExample::OnLootFound);

	// Start scavenging
	AActor* Target = ExampleCopilot;  // In real scenario, this would be a loot container
	ExampleCopilot->ScavengingComponent->StartScavenging(Target, 5.0f, LootTable);

	UE_LOG(LogTemp, Log, TEXT("AMadRunCopilotExample::DemonstrateScavenging - Loot find chance modifier: %.2f"), 
		ExampleCopilot->ScavengingComponent->GetLootFindChanceModifier());
}

void AMadRunCopilotExample::OnScavengeStarted(AActor* Target, float Duration)
{
	UE_LOG(LogTemp, Log, TEXT("Scavenging started on %s for %.2f seconds"), 
		*Target->GetName(), Duration);
}

void AMadRunCopilotExample::OnScavengeCompleted(AActor* Target, bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("Scavenging completed on %s - Success: %d"), 
		*Target->GetName(), bSuccess);
}

void AMadRunCopilotExample::OnLootFound(AActor* Target, FString LootType, int32 Amount)
{
	UE_LOG(LogTemp, Log, TEXT("Found %d x %s from %s"), 
		Amount, *LootType, *Target->GetName());
}

void AMadRunCopilotExample::LogCopilotStats()
{
	if (!ExampleCopilot)
	{
		return;
	}

	const FCopilotStats& Stats = ExampleCopilot->GetCopilotStats();

	UE_LOG(LogTemp, Log, TEXT("=== Copilot Stats ==="));
	UE_LOG(LogTemp, Log, TEXT("Mechanics Skill: %.1f"), Stats.MechanicsSkill);
	UE_LOG(LogTemp, Log, TEXT("Accuracy Skill: %.1f"), Stats.AccuracySkill);
	UE_LOG(LogTemp, Log, TEXT("Scavenging Skill: %.1f"), Stats.ScavengingSkill);
	UE_LOG(LogTemp, Log, TEXT(""));
	UE_LOG(LogTemp, Log, TEXT("=== Derived Stats ==="));
	UE_LOG(LogTemp, Log, TEXT("Repair Speed Multiplier: %.2f"), Stats.RepairSpeedMultiplier);
	UE_LOG(LogTemp, Log, TEXT("Accuracy Bonus: %.2f degrees"), Stats.AccuracyBonus);
	UE_LOG(LogTemp, Log, TEXT("Scavenging Find Chance: %.2f"), Stats.ScavengingFindChance);
}
