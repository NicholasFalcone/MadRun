#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MadRunCopilotStats.h"
#include "MadRunCopilotData.generated.h"

class USkeletalMesh;

UCLASS(BlueprintType, Blueprintable)
class MADRUN_API UMadRunCopilotData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Copilot")
	FText Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Copilot|Visual")
	TObjectPtr<USkeletalMesh> BodyMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Copilot|Stats")
	FCopilotStats Stats;
};
