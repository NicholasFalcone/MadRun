#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MadRunScavengerPoint.generated.h"

class AMadRunPawn;
class UMadRunEquipment;
class USphereComponent;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FMadRunScavengerEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scavenging")
	TObjectPtr<UMadRunEquipment> Equipment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scavenging", meta=(ClampMin="0.0"))
	float Weight = 1.0f;
};

UCLASS(Blueprintable)
class AMadRunScavengerPoint : public AActor
{
	GENERATED_BODY()

public:
	AMadRunScavengerPoint();

	bool IsVehicleInRange(const AMadRunPawn* Vehicle) const;
	UMadRunEquipment* GetOrSelectEquipment();
	bool TryEquip(AMadRunPawn* Vehicle, const UMadRunEquipment* OfferedEquipment);

	UFUNCTION(BlueprintPure, Category="Scavenging")
	bool IsConsumed() const { return bConsumed; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> Area;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> Marker;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scavenging", meta=(AllowPrivateAccess="true"))
	TArray<FMadRunScavengerEntry> PossibleEquipment;

	UPROPERTY(Transient)
	TObjectPtr<UMadRunEquipment> SelectedEquipment;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Scavenging", meta=(AllowPrivateAccess="true"))
	bool bConsumed = false;
};
