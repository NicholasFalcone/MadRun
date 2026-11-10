#include "MadRunScavengerPoint.h"
#include "MadRun.h"
#include "MadRunEquipment.h"
#include "MadRunPawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

AMadRunScavengerPoint::AMadRunScavengerPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	Area = CreateDefaultSubobject<USphereComponent>(TEXT("Area"));
	SetRootComponent(Area);
	Area->InitSphereRadius(400.0f);
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(Area);
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetRelativeScale3D(FVector(0.5f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Mesh.Succeeded())
	{
		Marker->SetStaticMesh(Mesh.Object);
	}
}

void AMadRunScavengerPoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMadRunScavengerPoint, bConsumed);
}

void AMadRunScavengerPoint::OnRep_Consumed()
{
	Marker->SetVisibility(!bConsumed);
}

bool AMadRunScavengerPoint::IsVehicleInRange(const AMadRunPawn* Vehicle) const
{
	return IsValid(Vehicle) && !Vehicle->IsActorBeingDestroyed() && !bConsumed
		&& FVector::DistSquared(Vehicle->GetActorLocation(), Area->GetComponentLocation())
			<= FMath::Square(Area->GetScaledSphereRadius());
}

UMadRunEquipment* AMadRunScavengerPoint::GetOrSelectEquipment()
{
	if (GetNetMode() == NM_Client || !HasAuthority())
	{
		UE_LOG(LogMadRun, Warning, TEXT("Scavenger offers must be selected by the server."));
		return nullptr;
	}
	if (bConsumed)
	{
		return nullptr;
	}
	if (IsValid(SelectedEquipment))
	{
		return SelectedEquipment;
	}

	double Total = 0.0;
	for (const FMadRunScavengerEntry& Entry : PossibleEquipment)
	{
		if (!FMath::IsFinite(Entry.Weight) || Entry.Weight < 0.0f || (Entry.Weight > 0.0f && !IsValid(Entry.Equipment)))
		{
			UE_LOG(LogMadRun, Error, TEXT("Scavenger point '%s' has an invalid equipment entry or weight."), *GetNameSafe(this));
			return nullptr;
		}
		Total += Entry.Weight;
	}
	if (Total <= 0.0)
	{
		UE_LOG(LogMadRun, Error, TEXT("Scavenger point '%s' has no equipment with a positive weight."), *GetNameSafe(this));
		return nullptr;
	}
	double Roll = FMath::FRand() * Total;
	for (const FMadRunScavengerEntry& Entry : PossibleEquipment)
	{
		if (Entry.Weight > 0.0f)
		{
			if (Roll < Entry.Weight)
			{
				SelectedEquipment = Entry.Equipment;
				return SelectedEquipment;
			}
			Roll -= Entry.Weight;
		}
	}
	UE_LOG(LogMadRun, Error, TEXT("Scavenger point '%s' failed to select equipment."), *GetNameSafe(this));
	return nullptr;
}

bool AMadRunScavengerPoint::TryEquip(AMadRunPawn* Vehicle, const UMadRunEquipment* OfferedEquipment)
{
	if (GetNetMode() == NM_Client || !HasAuthority())
	{
		UE_LOG(LogMadRun, Warning, TEXT("Scavenger equipment must be accepted by the server."));
		return false;
	}
	if (!IsVehicleInRange(Vehicle) || !IsValid(SelectedEquipment) || SelectedEquipment != OfferedEquipment)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Scavenger point '%s' cannot equip an unavailable offer."), *GetNameSafe(this));
		return false;
	}
	if (!Vehicle->Equip(SelectedEquipment))
	{
		return false;
	}
	bConsumed = true;
	OnRep_Consumed();
	ForceNetUpdate();
	return true;
}
