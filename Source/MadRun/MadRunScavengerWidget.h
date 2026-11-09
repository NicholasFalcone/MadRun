#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MadRunScavengerWidget.generated.h"

class UTextBlock;
class UMadRunEquipment;

UCLASS()
class UMadRunScavengerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	void ShowHint(const FText& KeyName);
	void ShowOffer(const UMadRunEquipment* Equipment, const FText& AcceptKey, const FText& RejectKey);
	void ShowFailure(const FText& RejectKey);
	void Clear();

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Message;
};
