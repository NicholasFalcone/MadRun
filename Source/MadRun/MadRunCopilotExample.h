// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MadRunCopilotExample.generated.h"

class ACopilot;

/**
 * Example class showing how to use the Copilot system
 * This demonstrates:
 * - Creating and initializing a copilot
 * - Setting up repair tasks
 * - Setting up combat scenarios
 * - Setting up scavenging tasks
 */
UCLASS()
class MADRUN_API AMadRunCopilotExample : public ACharacter
{
	GENERATED_BODY()

public:
	AMadRunCopilotExample();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Example copilot reference
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Copilot")
	TObjectPtr<ACopilot> ExampleCopilot;

	// Called to spawn and initialize the copilot
	UFUNCTION(BlueprintCallable, Category = "Copilot|Example")
	void SpawnAndInitializeCopilot();

	// Called to demonstrate repair functionality
	UFUNCTION(BlueprintCallable, Category = "Copilot|Example")
	void DemonstrateRepair();

	// Called to demonstrate combat functionality
	UFUNCTION(BlueprintCallable, Category = "Copilot|Example")
	void DemonstrateCombat();

	// Called to demonstrate scavenging functionality
	UFUNCTION(BlueprintCallable, Category = "Copilot|Example")
	void DemonstrateScavenging();

private:
	// Helper to log copilot stats
	void LogCopilotStats();

	// Event handlers for demonstration
	UFUNCTION()
	void OnRepairStarted(AActor* Target, float RepairDuration);

	UFUNCTION()
	void OnRepairCompleted(AActor* Target, bool bSuccess);

	UFUNCTION()
	void OnFireStarted(AActor* Target, float Damage);

	UFUNCTION()
	void OnFireCompleted(bool bHit);

	UFUNCTION()
	void OnScavengeStarted(AActor* Target, float Duration);

	UFUNCTION()
	void OnScavengeCompleted(AActor* Target, bool bSuccess);

	UFUNCTION()
	void OnLootFound(AActor* Target, FString LootType, int32 Amount);
};
