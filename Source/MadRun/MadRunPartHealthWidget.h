#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MadRunPartHealthWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class UMadRunPartHealthWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	void UpdateHealth(const FText& PartName, int32 Health, int32 MaxHealth);

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthBar;
};
