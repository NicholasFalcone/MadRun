#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MadRunPartHealth.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOutofWork);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRepaired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPartHealthChanged, int32, Health, int32, MaxHealth);

USTRUCT(BlueprintType)
struct FMadRunPartHealthState
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta=(ClampMin="1"))
	int32 MaxHealth = 10;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	int32 Health = 10;

	void Initialize() { Health = FMath::Max(MaxHealth, 1); }
	void Set(int32 NewHealth) { Health = FMath::Clamp(NewHealth, 0, FMath::Max(MaxHealth, 1)); }
};

UINTERFACE(BlueprintType)
class UMadRunPartHealth : public UInterface
{
	GENERATED_BODY()
};

class IMadRunPartHealth
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Health")
	int32 GetHealth() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Health")
	int32 GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Health")
	void SetHealth(int32 NewHealth);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Health")
	void TakeDamage(int32 DamageAmount);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Health")
	void DoRepair();
};
