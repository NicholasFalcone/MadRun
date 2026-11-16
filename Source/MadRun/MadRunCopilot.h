// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MadRunCopilotStats.h"
#include "MadRunCopilot.generated.h"

class UCopilotRepairComponent;
class UCopilotCombatComponent;
class UCopilotScavengingComponent;

/**
 * Copilot NPC Character
 * 
 * This character handles three main tasks:
 * - Repair: Fixes vehicle parts with speed influenced by mechanics skill
 * - Combat: Shoots at enemies with accuracy influenced by accuracy skill
 * - Scavenging: Searches for loot with find chance influenced by scavenging skill
 */
UCLASS()
class MADRUN_API ACopilot : public ACharacter
{
	GENERATED_BODY()

public:
	ACopilot();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Copilot Statistics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Copilot|Stats")
	FCopilotStats CopilotStats;

	// Repair Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Copilot|Components")
	TObjectPtr<UCopilotRepairComponent> RepairComponent;

	// Combat Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Copilot|Components")
	TObjectPtr<UCopilotCombatComponent> CombatComponent;

	// Scavenging Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Copilot|Components")
	TObjectPtr<UCopilotScavengingComponent> ScavengingComponent;

	// Get the copilot's current statistics
	UFUNCTION(BlueprintCallable, Category = "Copilot")
	const FCopilotStats& GetCopilotStats() const { return CopilotStats; }

	// Set all copilot statistics at once
	UFUNCTION(BlueprintCallable, Category = "Copilot")
	void SetCopilotStats(const FCopilotStats& NewStats);

	// Set individual skills
	UFUNCTION(BlueprintCallable, Category = "Copilot")
	void SetMechanicsSkill(float NewSkill);

	UFUNCTION(BlueprintCallable, Category = "Copilot")
	void SetAccuracySkill(float NewSkill);

	UFUNCTION(BlueprintCallable, Category = "Copilot")
	void SetScavengingSkill(float NewSkill);

private:
	void InitializeComponents();
};
