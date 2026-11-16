#include "MadRunVehicleMovementComponent.h"
#include "MadRunPawn.h"
#include "MadRunEquipment.h"
#include "Chaos/ChaosEngineInterface.h"
#include "Chaos/PBDSuspensionConstraintData.h"
#include "PhysicsEngine/BodyInstance.h"

void UMadRunVehicleMovementComponent::SetupVehicle(TUniquePtr<Chaos::FSimpleWheeledVehicle>& PVehicle)
{
	Super::SetupVehicle(PVehicle);

	// Chaos normally shares suspension setup through wheel class defaults; each vehicle needs its own copy.
	SuspensionConfigs.SetNum(PVehicle->Suspension.Num());
	const AMadRunPawn* Vehicle = Cast<AMadRunPawn>(GetOwner());
	const UMadRunTiresEquipment* Tires = IsValid(Vehicle) ? Vehicle->GetTiresEquipment() : nullptr;
	for (int32 Index = 0; Index < PVehicle->Suspension.Num(); ++Index)
	{
		SuspensionConfigs[Index] = PVehicle->Suspension[Index].Setup();
		PVehicle->Suspension[Index].SetupPtr = &SuspensionConfigs[Index];
		if (IsValid(Tires) && PVehicle->Wheels.IsValidIndex(Index))
		{
			PVehicle->Wheels[Index].FrictionMultiplier = Tires->FrictionForceMultiplier;
			SuspensionConfigs[Index].SetSuspensionMaxRaise(Tires->SuspensionMaxRaise);
			SuspensionConfigs[Index].SetSuspensionMaxDrop(Tires->SuspensionMaxDrop);
		}
	}
}

void UMadRunVehicleMovementComponent::ApplyTireStats(int32 WheelIndex, float Friction, float MaxRaise, float MaxDrop)
{
	FBodyInstance* Body = GetBodyInstance();
	if (!Body)
	{
		return;
	}

	FPhysicsCommand::ExecuteWrite(Body->GetPhysicsActor(), [&](const FPhysicsActorHandle& Chassis)
	{
		if (!VehicleSimulationPT || !VehicleSimulationPT->PVehicle
			|| !VehicleSimulationPT->PVehicle->Wheels.IsValidIndex(WheelIndex)
			|| !VehicleSimulationPT->PVehicle->Suspension.IsValidIndex(WheelIndex))
		{
			return;
		}

		VehicleSimulationPT->PVehicle->Wheels[WheelIndex].FrictionMultiplier = Friction;
		Chaos::FSimpleSuspensionConfig& Config = VehicleSimulationPT->PVehicle->Suspension[WheelIndex].AccessSetup();
		Config.SetSuspensionMaxRaise(MaxRaise);
		Config.SetSuspensionMaxDrop(MaxDrop);

		UChaosWheeledVehicleSimulation* Simulation = static_cast<UChaosWheeledVehicleSimulation*>(VehicleSimulationPT.Get());
		if (Simulation->ConstraintHandles.IsValidIndex(WheelIndex))
		{
			const FPhysicsConstraintHandle& Handle = Simulation->ConstraintHandles[WheelIndex];
			if (Handle.IsValid())
			{
				if (Chaos::FSuspensionConstraint* Constraint = static_cast<Chaos::FSuspensionConstraint*>(Handle.Constraint))
				{
					Constraint->SetMinLength(-MaxRaise);
					Constraint->SetMaxLength(MaxDrop);
				}
			}
		}
	});
}

#if WITH_DEV_AUTOMATION_TESTS
bool UMadRunVehicleMovementComponent::GetSimulatedTireStats(int32 WheelIndex, float& Friction, float& MaxRaise,
	float& MaxDrop, float& SpringRate, float& SpringPreload, float& TestGrip)
{
	FBodyInstance* Body = GetBodyInstance();
	if (!Body)
	{
		return false;
	}
	bool bFound = false;
	FPhysicsCommand::ExecuteRead(Body->GetPhysicsActor(), [&](const FPhysicsActorHandle& Chassis)
	{
		if (VehicleSimulationPT && VehicleSimulationPT->PVehicle
			&& VehicleSimulationPT->PVehicle->Wheels.IsValidIndex(WheelIndex)
			&& VehicleSimulationPT->PVehicle->Suspension.IsValidIndex(WheelIndex))
		{
			Friction = VehicleSimulationPT->PVehicle->Wheels[WheelIndex].FrictionMultiplier;
			const Chaos::FSimpleSuspensionConfig& Config = VehicleSimulationPT->PVehicle->Suspension[WheelIndex].Setup();
			MaxRaise = Config.SuspensionMaxRaise;
			MaxDrop = Config.SuspensionMaxDrop;
			SpringRate = Config.SpringRate;
			SpringPreload = Config.SpringPreload;
			Chaos::FSimpleWheelSim WheelCopy = VehicleSimulationPT->PVehicle->Wheels[WheelIndex];
			WheelCopy.SetWheelLoadForce(1000.0f);
			WheelCopy.SetSurfaceFriction(0.5f);
			WheelCopy.Simulate(1.0f / 60.0f);
			TestGrip = WheelCopy.AvailableGrip;
			bFound = true;
		}
	});
	return bFound;
}
#endif
