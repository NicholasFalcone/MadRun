#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "SuspensionSystem.h"
#include "MadRunVehicleMovementComponent.generated.h"

UCLASS()
class UMadRunVehicleMovementComponent : public UChaosWheeledVehicleMovementComponent
{
	GENERATED_BODY()

public:
	void ApplyTireStats(int32 WheelIndex, float Friction, float MaxRaise, float MaxDrop);

#if WITH_DEV_AUTOMATION_TESTS
	bool GetSimulatedTireStats(int32 WheelIndex, float& Friction, float& MaxRaise, float& MaxDrop,
		float& SpringRate, float& SpringPreload, float& TestGrip);
#endif

protected:
	virtual void SetupVehicle(TUniquePtr<Chaos::FSimpleWheeledVehicle>& PVehicle) override;

private:
	TArray<Chaos::FSimpleSuspensionConfig> SuspensionConfigs;
};
