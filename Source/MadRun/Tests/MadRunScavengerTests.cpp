#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunEquipment.h"
#include "MadRunScavengerPoint.h"
#include "MadRunScavengerWidget.h"
#include "OffroadCar/MadRunOffroadCar.h"
#include "Blueprint/UserWidget.h"
#include "Components/SphereComponent.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMadRunScavengerTest, "MadRun.Vehicle.Scavenging",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMadRunScavengerTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("World"), World))
	{
		return false;
	}
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();

	UClass* CarClass = LoadClass<AMadRunOffroadCar>(nullptr, TEXT("/Game/VehicleTemplate/Blueprints/OffroadCar/BP_OffroadCar_Pawn.BP_OffroadCar_Pawn_C"));
	AMadRunOffroadCar* Car = CarClass ? World->SpawnActor<AMadRunOffroadCar>(CarClass) : nullptr;
	AMadRunScavengerPoint* Point = World->SpawnActor<AMadRunScavengerPoint>();
	UMadRunEngineEquipment* First = NewObject<UMadRunEngineEquipment>(Point);
	UMadRunEngineEquipment* Second = NewObject<UMadRunEngineEquipment>(Point);
	FArrayProperty* Property = FindFProperty<FArrayProperty>(AMadRunScavengerPoint::StaticClass(), TEXT("PossibleEquipment"));
	if (TestNotNull(TEXT("Car"), Car) && TestNotNull(TEXT("Point"), Point) && TestNotNull(TEXT("Entries"), Property))
	{
		FScriptArrayHelper Entries(Property, Property->ContainerPtrToValuePtr<void>(Point));
		auto AddEntry = [&](UMadRunEquipment* Equipment, float Weight)
		{
			FMadRunScavengerEntry* Entry = reinterpret_cast<FMadRunScavengerEntry*>(Entries.GetRawPtr(Entries.AddValue()));
			Entry->Equipment = Equipment;
			Entry->Weight = Weight;
		};
		AddEntry(First, 0.0f);
		AddEntry(Second, 3.0f);
		TestTrue(TEXT("Car starts in range"), Point->IsVehicleInRange(Car));
		TestTrue(TEXT("Zero-weight entry is never selected"), Point->GetOrSelectEquipment() == Second);
		TestTrue(TEXT("Offer persists when revisited"), Point->GetOrSelectEquipment() == Second);
		AddExpectedError(TEXT("cannot equip an unavailable offer"), EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(TEXT("Wrong offer rejected"), Point->TryEquip(Car, First));
		TestFalse(TEXT("Rejected offer does not consume point"), Point->IsConsumed());
		Car->SetActorLocation(FVector(2000.0f, 0.0f, 0.0f));
		AddExpectedError(TEXT("cannot equip an unavailable offer"), EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(TEXT("Cannot accept outside range"), Point->TryEquip(Car, Second));
		TestFalse(TEXT("Leaving does not consume point"), Point->IsConsumed());
		Car->SetActorLocation(FVector::ZeroVector);
		TestTrue(TEXT("Accept equips offer"), Point->TryEquip(Car, Second));
		TestTrue(TEXT("Selected engine equipped"), Car->GetEngineEquipment() == Second);
		TestTrue(TEXT("Point consumed on success"), Point->IsConsumed());
		TestNull(TEXT("Consumed point has no offer"), Point->GetOrSelectEquipment());
		TestFalse(TEXT("Consumed point is not in range"), Point->IsVehicleInRange(Car));

		AMadRunScavengerPoint* EmptyPoint = World->SpawnActor<AMadRunScavengerPoint>();
		AddExpectedError(TEXT("has no equipment with a positive weight"), EAutomationExpectedErrorFlags::Contains, 1);
		TestNull(TEXT("Empty configuration fails"), EmptyPoint->GetOrSelectEquipment());
		TestFalse(TEXT("Empty point stays available"), EmptyPoint->IsConsumed());

		AMadRunScavengerPoint* InvalidPoint = World->SpawnActor<AMadRunScavengerPoint>();
		FScriptArrayHelper InvalidEntries(Property, Property->ContainerPtrToValuePtr<void>(InvalidPoint));
		FMadRunScavengerEntry* InvalidEntry = reinterpret_cast<FMadRunScavengerEntry*>(InvalidEntries.GetRawPtr(InvalidEntries.AddValue()));
		InvalidEntry->Equipment = NewObject<UMadRunWeaponEquipment>(InvalidPoint);
		InvalidEntry->Weight = 1.0f;
		UMadRunEquipment* InvalidOffer = InvalidPoint->GetOrSelectEquipment();
		AddExpectedError(TEXT("requires a valid equipment mesh"), EAutomationExpectedErrorFlags::Contains, 1);
		AddExpectedError(TEXT("cannot apply weapon equipment"), EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(TEXT("Failed equip does not succeed"), InvalidPoint->TryEquip(Car, InvalidOffer));
		TestFalse(TEXT("Failed equip does not consume point"), InvalidPoint->IsConsumed());
		TestTrue(TEXT("Failed offer remains available"), InvalidPoint->GetOrSelectEquipment() == InvalidOffer);

		int32 SelectedFirst = 0;
		for (int32 Index = 0; Index < 1000; ++Index)
		{
			AMadRunScavengerPoint* Sample = World->SpawnActor<AMadRunScavengerPoint>();
			FScriptArrayHelper SampleEntries(Property, Property->ContainerPtrToValuePtr<void>(Sample));
			FMadRunScavengerEntry* Entry = reinterpret_cast<FMadRunScavengerEntry*>(SampleEntries.GetRawPtr(SampleEntries.AddValue()));
			Entry->Equipment = First;
			Entry->Weight = 3.0f;
			Entry = reinterpret_cast<FMadRunScavengerEntry*>(SampleEntries.GetRawPtr(SampleEntries.AddValue()));
			Entry->Equipment = Second;
			Entry->Weight = 7.0f;
			SelectedFirst += Sample->GetOrSelectEquipment() == First ? 1 : 0;
			Sample->Destroy();
		}
		TestTrue(FString::Printf(TEXT("Weight 3/10 selected between 20%% and 40%% (%d/1000)"), SelectedFirst),
			SelectedFirst >= 200 && SelectedFirst <= 400);

		UMadRunScavengerWidget* Widget = CreateWidget<UMadRunScavengerWidget>(World, UMadRunScavengerWidget::StaticClass());
		if (TestNotNull(TEXT("Scavenging UI"), Widget))
		{
			UTextBlock* Message = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("Message")));
			if (TestNotNull(TEXT("UI message"), Message))
			{
				Widget->ShowHint(FText::FromString(TEXT("E")));
				TestTrue(TEXT("Hint is visible"), Widget->GetVisibility() == ESlateVisibility::HitTestInvisible);
				TestTrue(TEXT("Hint displays the interaction key"), Message->GetText().ToString().Contains(TEXT("E")));
				TestFalse(TEXT("Text wrapping does not depend on the panel's current width"), Message->GetAutoWrapText());
				TestEqual(TEXT("Long descriptions use a fixed wrap width"), Message->GetWrapTextAt(), 500.0f);
				Second->Name = FText::FromString(TEXT("Test engine"));
				Widget->ShowOffer(Second, FText::FromString(TEXT("Y")), FText::FromString(TEXT("N")));
				TestTrue(TEXT("Offer shows item and keys"), Message->GetText().ToString().Contains(TEXT("Test engine"))
					&& Message->GetText().ToString().Contains(TEXT("Y: Equip"))
					&& Message->GetText().ToString().Contains(TEXT("N: Cancel")));
				Widget->Clear();
				TestTrue(TEXT("UI can be hidden"), Widget->GetVisibility() == ESlateVisibility::Collapsed);
			}
		}
	}
	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
