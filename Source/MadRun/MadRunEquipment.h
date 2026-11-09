#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MadRunEquipment.generated.h"

class UStaticMesh;

UCLASS(Abstract, BlueprintType, Blueprintable)
class UMadRunEquipment : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment")
	FText Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment", meta=(MultiLine="true"))
	FText Description;
};

UCLASS(BlueprintType, Blueprintable)
class UMadRunEngineEquipment : public UMadRunEquipment
{
	GENERATED_BODY()
};

UCLASS(BlueprintType, Blueprintable)
class UMadRunWeaponEquipment : public UMadRunEquipment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment|Visual")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment|Visual")
	FTransform MeshTransform = FTransform(FRotator::ZeroRotator, FVector(50.0f, 0.0f, 0.0f));
};

UCLASS(BlueprintType, Blueprintable)
class UMadRunTiresEquipment : public UMadRunEquipment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment|Visual")
	TObjectPtr<UStaticMesh> Mesh;
};

UCLASS(BlueprintType, Blueprintable)
class UMadRunFuelTankEquipment : public UMadRunEquipment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment|Visual")
	TObjectPtr<UStaticMesh> Mesh;
};
