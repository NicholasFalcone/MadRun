#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "MadRunPartHealthDisplay.generated.h"

UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class UMadRunPartHealthDisplay : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UMadRunPartHealthDisplay();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SetPartName(const FText& Name);
	void SetHeightAbovePart(float Height);
	void ShowHealth(int32 Health, int32 MaxHealth);

private:
	void HideHealth();
	void UpdateWorldTransform();

	UPROPERTY(EditAnywhere, Category="Health Display", meta=(ClampMin="0.01", Units="s"))
	float DisplayDuration = 3.0f;

	UPROPERTY(EditAnywhere, Category="Health Display", meta=(ClampMin="0.0", Units="cm"))
	float HeightAbovePart = 100.0f;

	UPROPERTY(EditAnywhere, Category="Health Display")
	FText PartName;

	FTimerHandle HideTimer;
};
