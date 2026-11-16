// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunCopilotScavengingComponent.h"
#include "MadRunCopilotStats.h"

UCopilotScavengingComponent::UCopilotScavengingComponent()
{
	PrimaryComponentTick.TickInterval = 0.1f;
	PrimaryComponentTick.bCanEverTick = true;
}

void UCopilotScavengingComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickEnabled(false);
}

void UCopilotScavengingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentState != EScavengingState::Searching || !CurrentTarget)
	{
		return;
	}

	ElapsedSearchTime += DeltaTime;

	if (ElapsedSearchTime >= TotalSearchDuration)
	{
		CompleteScavenging(true);
	}
}

void UCopilotScavengingComponent::Initialize(const FCopilotStats& InStats)
{
	CopilotStats = InStats;
}

void UCopilotScavengingComponent::StartScavenging(AActor* InTarget, float SearchDuration, const TArray<FLootTable>& LootTable)
{
	if (!InTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCopilotScavengingComponent::StartScavenging - Invalid target"));
		return;
	}

	// Cancel any ongoing scavenging
	if (CurrentState == EScavengingState::Searching)
	{
		CancelScavenging();
	}

	CurrentTarget = InTarget;
	CurrentState = EScavengingState::Searching;
	TotalSearchDuration = SearchDuration;
	ElapsedSearchTime = 0.0f;
	CurrentLootTable = LootTable;
	FoundLoot.Empty();

	SetComponentTickEnabled(true);

	OnScavengeStarted.Broadcast(CurrentTarget, SearchDuration);

	UE_LOG(LogTemp, Log,
		TEXT("UCopilotScavengingComponent::StartScavenging - Target: %s, Duration: %.2f seconds, Loot find chance modifier: %.2f%%"),
		*InTarget->GetName(), SearchDuration, GetLootFindChanceModifier() * 100.0f);
}

void UCopilotScavengingComponent::CancelScavenging()
{
	if (CurrentState != EScavengingState::Searching)
	{
		return;
	}

	CurrentState = EScavengingState::Cancelled;
	SetComponentTickEnabled(false);

	OnScavengeCompleted.Broadcast(CurrentTarget, false);

	UE_LOG(LogTemp, Log, TEXT("UCopilotScavengingComponent::CancelScavenging - Scavenging cancelled at %.2f%% progress"),
		GetScavengingProgress() * 100.0f);

	CurrentTarget = nullptr;
	FoundLoot.Empty();
}

float UCopilotScavengingComponent::GetScavengingProgress() const
{
	if (TotalSearchDuration <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(ElapsedSearchTime / TotalSearchDuration, 0.0f, 1.0f);
}

float UCopilotScavengingComponent::GetLootFindChanceModifier() const
{
	return CopilotStats.GetLootFindChance();
}

void UCopilotScavengingComponent::CompleteScavenging(bool bSuccess)
{
	CurrentState = EScavengingState::Completed;
	SetComponentTickEnabled(false);

	if (bSuccess)
	{
		RollForLoot();
	}

	OnScavengeCompleted.Broadcast(CurrentTarget, bSuccess);

	UE_LOG(LogTemp, Log, TEXT("UCopilotScavengingComponent::CompleteScavenging - Target: %s, Success: %d, Items found: %d"),
		*CurrentTarget->GetName(), bSuccess, FoundLoot.Num());

	CurrentTarget = nullptr;
}

void UCopilotScavengingComponent::RollForLoot()
{
	const float ChanceModifier = GetLootFindChanceModifier();

	for (const FLootTable& Loot : CurrentLootTable)
	{
		// Apply copilot's scavenging skill to loot chances
		const float AdjustedChance = Loot.DropChance * ChanceModifier;

		if (FMath::FRand() * 100.0f <= AdjustedChance)
		{
			const int32 Amount = FMath::RandRange(Loot.MinAmount, Loot.MaxAmount);
			FoundLoot.Add(Loot.LootType, Amount);

			OnLootFound.Broadcast(CurrentTarget, Loot.LootType, Amount);

			UE_LOG(LogTemp, Log, TEXT("UCopilotScavengingComponent::RollForLoot - Found %d x %s (Original chance: %.2f%%, Adjusted: %.2f%%)"),
				Amount, *Loot.LootType, Loot.DropChance, AdjustedChance);
		}
	}
}
