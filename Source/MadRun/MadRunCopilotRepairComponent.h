// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MadRunCopilotStats.h"
#include "MadRunCopilotRepairComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRepairStarted, AActor*, Target, float, RepairDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRepairCompleted, AActor*, Target, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRepairCancelled, AActor*, Target, float, AmountRepaired);

UENUM(BlueprintType)
enum class ERepairState : uint8
{
	Idle,
	Repairing,
	Completed,
	Cancelled
};

UCLASS(ClassGroup = (Copilot), meta = (BlueprintSpawnableComponent))
class MADRUN_API UCopilotRepairComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCopilotRepairComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Initialize the component with copilot stats
	UFUNCTION(BlueprintCallable, Category = "Copilot|Repair")
	void Initialize(const FCopilotStats& InStats);

	// Start repairing a target actor
	// BaseRepairTime: time needed without any skill modifications (in seconds)
	// MaxRepairAmount: maximum health to repair (usually 100 for health value)
	UFUNCTION(BlueprintCallable, Category = "Copilot|Repair")
	void StartRepair(AActor* InTarget, float BaseRepairTime, float MaxRepairAmount = 100.0f);

	// Cancel ongoing repair
	UFUNCTION(BlueprintCallable, Category = "Copilot|Repair")
	void CancelRepair();

	// Get current repair state
	UFUNCTION(BlueprintCallable, Category = "Copilot|Repair")
	ERepairState GetRepairState() const { return CurrentState; }

	// Get repair progress (0.0 - 1.0)
	UFUNCTION(BlueprintCallable, Category = "Copilot|Repair")
	float GetRepairProgress() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Copilot|Repair")
	FOnRepairStarted OnRepairStarted;

	UPROPERTY(BlueprintAssignable, Category = "Copilot|Repair")
	FOnRepairCompleted OnRepairCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Copilot|Repair")
	FOnRepairCancelled OnRepairCancelled;

private:
	void CompleteRepair(bool bSuccess);

	FCopilotStats CopilotStats;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget;

	ERepairState CurrentState = ERepairState::Idle;

	float ElapsedRepairTime = 0.0f;
	float TotalRepairTime = 0.0f;
	float MaxRepairAmount = 0.0f;
	float AdjustedRepairTime = 0.0f;
};
