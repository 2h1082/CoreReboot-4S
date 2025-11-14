#include "UI/MainMenu/GameSaveWidget.h"
#include "UI/MainMenu/MainMenuWidget.h"
#include "UI/Common/ButtonWidget.h"
#include "UI/MainMenu/EmptySlotWidget.h"
#include "UI/Common/ConfirmWidget.h"
#include "UI/MainMenu/SaveSlotWidget.h"
#include "Game/SaveGame/C4MetaSaveGame.h"
#include "Game/SaveGame/SaveGameManager.h"
#include "Game/GameMode/C4MenuGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Border.h"

void UGameSaveWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackButton)
	{
		BackButton->OnClicked().AddDynamic(this, &UGameSaveWidget::HandleCloseWindow);
	}
	if (DeleteButton)
	{
		DeleteButton->OnClicked().AddDynamic(this, &UGameSaveWidget::HandleDeleteButtonClicked);
	}

	if (SaveSlot0)
		SaveSlot0->OnClicked().AddDynamic(this, &UGameSaveWidget::HandleEmptySlot0);
	if (SaveSlot1)
		SaveSlot1->OnClicked().AddDynamic(this, &UGameSaveWidget::HandleEmptySlot1);
	if (SaveSlot2)
		SaveSlot2->OnClicked().AddDynamic(this, &UGameSaveWidget::HandleEmptySlot2);
	if (SaveSlot3)
		SaveSlot3->OnClicked().AddDynamic(this, &UGameSaveWidget::HandleEmptySlot3);

	SaveSlotButtons = { SaveSlot0, SaveSlot1, SaveSlot2, SaveSlot3 };

	SaveGameManager = GetGameInstance()->GetSubsystem<USaveGameManager>();
	LoadSaveSlots();
}

void UGameSaveWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

}

void UGameSaveWidget::OpenWindow()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UGameSaveWidget::HandleCloseWindow()
{
	MainMenuWidgetRef->ShowMenuButtons();
	SetVisibility(ESlateVisibility::Hidden);
}

void UGameSaveWidget::HandleEmptySlotSelected(int32 SlotIndex)
{
	SelectedSlotIndex = SlotIndex;

	ConfirmWidget = CreateWidget<UConfirmWidget>(GetWorld(), ConfirmWidgetClass);
	if (ConfirmWidget)
	{
		ConfirmWidget->AddToViewport();
		ConfirmWidget->Confirm(FText::FromString(FString::Printf(TEXT("%d번 슬롯에 새로운 게임을 시작하시겠습니까?"), SlotIndex)));
		ConfirmWidget->OnYes.BindDynamic(this, &UGameSaveWidget::HandleStartNewGame);
		ConfirmWidget->OnNo.BindDynamic(this, &UGameSaveWidget::HandleCloseConfirmWidget);
	}
}

void UGameSaveWidget::HandleDeleteButtonClicked()
{
	if (SelectedSlotIndex < 0 || SelectedSlotIndex >= SaveSlotButtons.Num()) return;

	if (!SaveSlotsMap.Contains(SelectedSlotIndex))
	{
		LogSaveSlotsInfo();
		return;
	}

	ConfirmWidget = CreateWidget<UConfirmWidget>(GetWorld(), ConfirmWidgetClass);
	if (ConfirmWidget)
	{
		ConfirmWidget->AddToViewport();
		ConfirmWidget->Confirm(FText::FromString(FString::Printf(TEXT("Delete slot %d?"), SelectedSlotIndex)));
		ConfirmWidget->OnYes.BindDynamic(this, &UGameSaveWidget::HandleDeleteSlot);
		ConfirmWidget->OnNo.BindDynamic(this, &UGameSaveWidget::HandleCloseConfirmWidget);
	}
}

void UGameSaveWidget::HandleDeleteSlot()
{
	UE_LOG(LogTemp, Log, TEXT("[SaveSlot] Attempting to delete slot at index: %d"), SelectedSlotIndex);

	CreateEmptySlot(SelectedSlotIndex);

	if (SaveGameManager)
	{
		SaveGameManager->DeleteSlot(FString::FromInt(SelectedSlotIndex));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSlot] SaveGameManager is nullptr."));
	}

	HandleCloseConfirmWidget();

	LogSaveSlotsInfo();
}

void UGameSaveWidget::HandleStartNewGame()
{
	HandleCloseConfirmWidget();

	if (!SaveSlotButtons.IsValidIndex(SelectedSlotIndex)) return;

	UEmptySlotWidget* TargetButton = SaveSlotButtons[SelectedSlotIndex];
	if (!TargetButton) return;
	
	switch (SelectedSlotIndex)
	{
	case 0:
		TargetButton->OnClicked().RemoveDynamic(this, &UGameSaveWidget::HandleEmptySlot0);
		break;
	case 1:
		TargetButton->OnClicked().RemoveDynamic(this, &UGameSaveWidget::HandleEmptySlot1);
		break;
	case 2:
		TargetButton->OnClicked().RemoveDynamic(this, &UGameSaveWidget::HandleEmptySlot2);
		break;
	case 3:
		TargetButton->OnClicked().RemoveDynamic(this, &UGameSaveWidget::HandleEmptySlot3);
		break;
	default:
		break;
	}

	UBorder* ParentBorder = Cast<UBorder>(TargetButton->GetParent());
	if (!ParentBorder) return;

	if (SaveSlotsMap.Contains(SelectedSlotIndex))
	{
		SaveSlotsMap[SelectedSlotIndex]->RemoveFromParent();
		SaveSlotsMap.Remove(SelectedSlotIndex);
	}

	USaveSlotWidget* NewSlotWidget = CreateWidget<USaveSlotWidget>(GetWorld(), SaveSlotWidgetClass);
	if (!NewSlotWidget) return;

	NewSlotWidget->SetVisibility(ESlateVisibility::Collapsed);
	ParentBorder->SetContent(NewSlotWidget);
	NewSlotWidget->PlayCreateAnimation();
	NewSlotWidget->SetVisibility(ESlateVisibility::Visible);

	SaveGameManager->CreateSlot(FString::FromInt(SelectedSlotIndex));
	NewSlotWidget->UpdateSlotInfo(SaveGameManager->GetSaveMetaDataByIndex(SelectedSlotIndex));
	NewSlotWidget->OnSlotClicked.BindUObject(this, &UGameSaveWidget::HandleSaveSlotClicked);
	SaveSlotsMap.Add(SelectedSlotIndex, NewSlotWidget);

	FTimerHandle TimerHandle;
	FTimerDelegate TimerDel = FTimerDelegate::CreateUFunction(this, FName("RequestEnterGame"), SelectedSlotIndex);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 0.5f, false);
}

void UGameSaveWidget::LoadSaveSlots()
{
	if (!SaveGameManager) return;
	SaveGameManager->LoadMeta();

	UC4MetaSaveGame* Meta = SaveGameManager->GetMetaSave();
	if (!Meta) return;

	SaveSlotsMap.Empty();

	for (int32 Index = 0; Index < SaveSlotButtons.Num(); ++Index)
	{
		if (!SaveSlotButtons.IsValidIndex(Index)) continue;

		UEmptySlotWidget* TargetButton = SaveSlotButtons[Index];
		if (!TargetButton) continue;

		UBorder* ParentBorder = Cast<UBorder>(TargetButton->GetParent());
		if (!ParentBorder) continue;

		FSaveSlotMetaData MetaData = SaveGameManager->GetSaveMetaDataByIndex(Index);
		if (!MetaData.IsValid())
		{
			ParentBorder->SetContent(TargetButton);
			continue;
		}

		USaveSlotWidget* SlotWidget = CreateWidget<USaveSlotWidget>(GetWorld(), SaveSlotWidgetClass);
		if (!SlotWidget) continue;

		ParentBorder->SetContent(SlotWidget);
		SlotWidget->UpdateSlotInfo(MetaData);
		SlotWidget->OnSlotClicked.BindUObject(this, &UGameSaveWidget::HandleSaveSlotClicked);

		SaveSlotsMap.Add(Index, SlotWidget);
	}
}

void UGameSaveWidget::HandleSaveSlotClicked(int32 SlotIndex)
{
	if (SlotIndex == SelectedSlotIndex)
	{
		RequestEnterGame(SlotIndex);
	}
	else
	{
		SelectedSlotIndex = SlotIndex;
	}
}

void UGameSaveWidget::HandleCloseConfirmWidget()
{
	if (ConfirmWidget)
	{
		ConfirmWidget->RemoveFromParent();
		ConfirmWidget = nullptr;
	}
}

void UGameSaveWidget::RequestEnterGame(int32 SlotIndex)
{
	AC4MenuGameMode* GM = Cast<AC4MenuGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->OpenInGameLevel(SlotIndex);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Game mode is not valid."));
	}
}

void UGameSaveWidget::LogSaveSlotsInfo() const
{
	UE_LOG(LogTemp, Log, TEXT("[SaveSlot] ===== SaveSlotsMap Debug Info ====="));

	for (const auto& Elem : SaveSlotsMap)
	{
		if (Elem.Value)
		{
			UE_LOG(LogTemp, Log, TEXT("[SaveSlot] Index %d → Valid Widget: %s"), Elem.Key, *Elem.Value->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[SaveSlot] Index %d → nullptr"), Elem.Key);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[SaveSlot] Total SaveSlots: %d"), SaveSlotsMap.Num());
}

void UGameSaveWidget::CreateEmptySlot(int32 SlotIndex)
{
	if (!SaveSlotButtons.IsValidIndex(SlotIndex)) return;

	USaveSlotWidget* ExistingSlot = SaveSlotsMap.FindRef(SlotIndex);
	if (!ExistingSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSlot] No SaveSlotWidget found at index %d"), SlotIndex);
		return;
	}

	UBorder* ParentBorder = Cast<UBorder>(ExistingSlot->GetParent());
	if (!ParentBorder)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSlot] ParentBorder not found for SaveSlot at index %d"), SlotIndex);
		return;
	}

	ExistingSlot->OnSlotClicked.Unbind();
	ExistingSlot->RemoveFromParent();
	SaveSlotsMap.Remove(SlotIndex);

	UEmptySlotWidget* NewEmptySlot = CreateWidget<UEmptySlotWidget>(GetWorld(), EmptySlotWidgetClass);
	if (!NewEmptySlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSlot] Failed to create new EmptySlotWidget at index %d"), SlotIndex);
		return;
	}

	switch (SlotIndex)
	{
	case 0:
		NewEmptySlot->OnClicked().AddDynamic(this, &UGameSaveWidget::HandleEmptySlot0);
		break;
	case 1:
		NewEmptySlot->OnClicked().AddDynamic(this, &UGameSaveWidget::HandleEmptySlot1);
		break;
	case 2:
		NewEmptySlot->OnClicked().AddDynamic(this, &UGameSaveWidget::HandleEmptySlot2);
		break;
	case 3:
		NewEmptySlot->OnClicked().AddDynamic(this, &UGameSaveWidget::HandleEmptySlot3);
		break;
	default:
		break;
	}

	ParentBorder->SetContent(NewEmptySlot);
	SaveSlotButtons[SlotIndex] = NewEmptySlot;
}


