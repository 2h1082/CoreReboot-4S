#include "UI/MainMenu/SaveSlotWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void USaveSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if(SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &USaveSlotWidget::OnSlotButtonClicked);
	}
}

void USaveSlotWidget::OnSlotButtonClicked()
{
	if (OnSlotClicked.IsBound())
	{
		FString SlotNameString = SlotNameText->GetText().ToString();
		int32 SlotIndex = FCString::Atoi(*SlotNameString);
		OnSlotClicked.Execute(SlotIndex);

		UE_LOG(LogTemp, Warning, TEXT("Slot %d clicked"), SlotIndex);
	}
}

void USaveSlotWidget::UpdateSlotInfo(FSaveSlotMetaData SlotData)
{
	FDateTime Time = SlotData.SaveTime;
	FString TimeString = Time.ToString(TEXT("%Y-%m-%d %H:%M"));
	FText SaveText = FText::FromString(TimeString);

	SaveTimeText->SetText(SaveText);
	SlotNameText->SetText(FText::FromString(SlotData.SlotName));
	PlayTimeText->SetText(FText::FromString(SlotData.TotalGameTime));
	SeasonText->SetText(FText::FromString(SlotData.Season));
	SetSlotThumbnail(SlotData.Season);
}

void USaveSlotWidget::PlayCreateAnimation()
{
	if (OnCreateAnimation)
	{
		PlayAnimation(OnCreateAnimation);
	}
}

void USaveSlotWidget::SetSlotThumbnail(const FString& Season)
{
	UE_LOG(LogTemp, Warning, TEXT("SetSlotThumbnail Called - Season: [%s]"), *Season);

	FString TrimmedSeason = Season.TrimStartAndEnd();
	UE_LOG(LogTemp, Warning, TEXT("Trimmed Season: [%s], Length: %d"), *TrimmedSeason, TrimmedSeason.Len());

	if (!SlotThumbnail)
	{
		UE_LOG(LogTemp, Warning, TEXT("SlotThumbnail is nullptr!"));
		return;
	}

	if (TrimmedSeason.Equals(TEXT("풍요기"), ESearchCase::CaseSensitive))
	{
		UE_LOG(LogTemp, Warning, TEXT("Matched Season: 풍요기"));
		if (!BountifulSeasonImage)
		{
			UE_LOG(LogTemp, Warning, TEXT("BountifulSeasonImage is nullptr!"));
		}
		SlotThumbnail->SetBrushFromTexture(BountifulSeasonImage);
	}
	else if (TrimmedSeason.Equals(TEXT("혹한기"), ESearchCase::CaseSensitive))
	{
		UE_LOG(LogTemp, Warning, TEXT("Matched Season: 혹한기"));
		if (!FrostSeasonImage)
		{
			UE_LOG(LogTemp, Warning, TEXT("FrostSeasonImage is nullptr!"));
		}
		SlotThumbnail->SetBrushFromTexture(FrostSeasonImage);
	}
	else if (TrimmedSeason.Equals(TEXT("우기"), ESearchCase::CaseSensitive))
	{
		UE_LOG(LogTemp, Warning, TEXT("Matched Season: 우기"));
		if (!RainySeasonImage)
		{
			UE_LOG(LogTemp, Warning, TEXT("RainySeasonImage is nullptr!"));
		}
		SlotThumbnail->SetBrushFromTexture(RainySeasonImage);
	}
	else if (TrimmedSeason.Equals(TEXT("건기"), ESearchCase::CaseSensitive))
	{
		UE_LOG(LogTemp, Warning, TEXT("Matched Season: 건기"));
		if (!DrySeasonImage)
		{
			UE_LOG(LogTemp, Warning, TEXT("DrySeasonImage is nullptr!"));
		}
		SlotThumbnail->SetBrushFromTexture(DrySeasonImage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Season did not match any known case. Using DefaultImage."));
		if (!DefaultImage)
		{
			UE_LOG(LogTemp, Warning, TEXT("DefaultImage is nullptr!"));
		}
		SlotThumbnail->SetBrushFromTexture(DefaultImage);
	}
}
