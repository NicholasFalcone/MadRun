// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MadRunCopilotStats.h"
#include "MadRunCopilotScavengingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScavengeStarted, AActor*, Target, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScavengeCompleted, AActor*, Target, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLootFound, AActor*, Target, FString, LootType, int32, Amount);

UENUM(BlueprintType)
enum class EScavengingState : uint8
{
	Idle,
	Searching,
	Completed,
	Cancelled
};

USTRUCT(BlueprintType)
struct FLootTable
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scavenging")
	FString LootType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scavenging", meta = (ClampMin = "0", ClampMax = "100"))
	float DropChance = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scavenging")
	int32 MinAmount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scavenging")
	int32 MaxAmount = 10;
};

UCLASS(ClassGroup = (Copilot), meta = (BlueprintSpawnableComponent))
class MADRUN_API UCopilotScavengingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCopilotScavengingComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Initialize the component with copilot stats
	UFUNCTION(BlueprintCallable, Category = "Copilot|Scavenging")
	void Initialize(const FCopilotStats& InStats);

	// Start searching for loot at a location
	UFUNCTION(BlueprintCallable, Category = "Copilot|Scavenging")
	void StartScavenging(AActor* InTarget, float SearchDuration, const TArray<FLootTable>& LootTable);

	// Cancel ongoing scavenging
	UFUNCTION(BlueprintCallable, Category = "Copilot|Scavenging")
	void CancelScavenging();

	// Get current scavenging state
	UFUNCTION(BlueprintCallable, Category = "Copilot|Scavenging")
	EScavengingState GetScavengingState() const { return CurrentState; }

	// Get scavenging progress (0.0 - 1.0)
	UFUNCTION(BlueprintCallable, Category = "Copilot|Scavenging")
	float GetScavengingProgress() const;

	// Get the loot find chance modifier based on scavenging skill
	UFUNCTION(BlueprintCallable, Category = "Copilot|Scavenging")
	float GetLootFindChanceModifier() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Copilot|Scavenging")
	FOnScavengeStarted OnScavengeStarted;

	UPROPERTY(BlueprintAssignable, Category = "Copilot|Scavenging")
	FOnScavengeCompleted OnScavengeCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Copilot|Scavenging")
	FOnLootFound OnLootFound;

private:
	void CompleteScavenging(bool bSuccess);
	void RollForLoot();

	FCopilotStats CopilotStats;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget;

	EScavengingState CurrentState = EScavengingState::Idle;

	float ElapsedSearchTime = 0.0f;
	float TotalSearchDuration = 0.0f;

	TArray<FLootTable> CurrentLootTable;

	UPROPERTY()
	TMap<FString, int32> FoundLoot;
};
