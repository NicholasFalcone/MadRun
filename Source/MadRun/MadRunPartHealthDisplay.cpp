#include "MadRunPartHealthDisplay.h"
#include "MadRunPartHealthWidget.h"
#include "MadRun.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UMadRunPartHealthDisplay::UMadRunPartHealthDisplay()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	SetWidgetSpace(EWidgetSpace::World);
	SetWidgetClass(UMadRunPartHealthWidget::StaticClass());
	SetDrawSize(FVector2D(200.0f, 60.0f));
	SetPivot(FVector2D(0.5f, 0.5f));
	SetRelativeScale3D(FVector(0.3f));
	SetAbsolute(false, true, true);
	SetTwoSided(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetVisibility(false);
}

void UMadRunPartHealthDisplay::SetHeightAbovePart(float Height)
{
	HeightAbovePart = FMath::Max(0.0f, Height);
}

void UMadRunPartHealthDisplay::SetPartName(const FText& Name)
{
	PartName = Name;
}

void UMadRunPartHealthDisplay::ShowHealth(int32 Health, int32 MaxHealth)
{
	UWorld* World = GetWorld();
	if (!World || World->IsNetMode(NM_DedicatedServer))
	{
		return;
	}
	InitWidget();
	UMadRunPartHealthWidget* HealthWidget = Cast<UMadRunPartHealthWidget>(GetUserWidgetObject());
	if (!HealthWidget)
	{
		UE_LOG(LogMadRun, Error, TEXT("Cannot show health for '%s': widget is unavailable or has an incompatible class."), *GetNameSafe(GetOwner()));
		return;
	}
	HealthWidget->UpdateHealth(PartName, Health, MaxHealth);
	UpdateWorldTransform();
	SetVisibility(true);
	World->GetTimerManager().SetTimer(HideTimer, this, &UMadRunPartHealthDisplay::HideHealth,
		FMath::Max(0.01f, DisplayDuration), false);
}

void UMadRunPartHealthDisplay::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsVisible())
	{
		UpdateWorldTransform();
	}
}

void UMadRunPartHealthDisplay::UpdateWorldTransform()
{
	if (const USceneComponent* Parent = GetAttachParent())
	{
		SetWorldLocation(Parent->GetComponentLocation() + FVector::UpVector * FMath::Max(0.0f, HeightAbovePart));
	}
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector Direction = Camera->GetCameraLocation() - GetComponentLocation();
		if (!Direction.IsNearlyZero())
		{
			SetWorldRotation(Direction.Rotation());
		}
	}
}

void UMadRunPartHealthDisplay::HideHealth()
{
	SetVisibility(false);
}

void UMadRunPartHealthDisplay::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
	}
	Super::EndPlay(EndPlayReason);
}
