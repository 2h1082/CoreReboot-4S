#include "UI/InGame/SurvivalHUD.h"
#include "Blueprint/UserWidget.h"
#include "Character/Components/BaseStatusComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Inventory/UI/InventoryContainerWidget.h"


void ASurvivalHUD::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InGameWidget = CreateAndAddWidget<UDefaultInGameWidget>(InGameWidgetClass, 0, ESlateVisibility::Visible);

	PauseWidget = CreateAndAddWidget<UPauseWidget>(PauseWidgetClass, 12, ESlateVisibility::Collapsed);
	PauseWidget->OnResumeRequested.BindUObject(this, &ASurvivalHUD::HandlePauseToggle);

	InventoryContainerWidget = CreateAndAddWidget(InventoryContainerWidgetClass, 0, ESlateVisibility::Visible);
}


void ASurvivalHUD::BeginPlay()
{
	Super::BeginPlay();

}


void ASurvivalHUD::BindGameOverWidget(UBaseStatusComponent* BaseStatComp)
{
	if (!BaseStatComp) return;
	GameOverWidget = CreateAndAddWidget<UGameOverWidget>(GameOverWidgetClass, 20, ESlateVisibility::Hidden);

	BaseStatComp->OnDeathState.AddLambda([this]()
		{
			if (GameOverWidget)
			{
				HandleGameOverToggle();
			}
		});
}



void ASurvivalHUD::ShowLoading()
{
	ULoadingWidget* LoadingWidget = CreateAndAddWidget<ULoadingWidget>(LoadingWidgetClass, 11, ESlateVisibility::Visible);
	ShowWidgetOnly(LoadingWidget);
}

void ASurvivalHUD::ShowMessage(const FText& InText, float Duration)
{
	NotificationWidget = CreateAndAddWidget<UNotificationWidget>(NotificationWidgetClass, 100, ESlateVisibility::Collapsed);

	NotificationWidget->ShowNotification(InText, Duration);
}

void ASurvivalHUD::PlayEndingSequence()
{
	EndingWidget = CreateAndAddWidget<UEndingSummaryWidget>(EndingSummaryWidgetClass, 30, ESlateVisibility::Visible);
	SetInputMode(ESurvivalInputMode::UIOnly, EndingWidget, true);
}

void ASurvivalHUD::HandlePauseToggle()
{
	const bool bIsVisible = PauseWidget->GetVisibility() == ESlateVisibility::Visible;

	if (bIsVisible)
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
		SetInputMode(ESurvivalInputMode::GameOnly, nullptr, false);
	}
	else
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0001f);
		SetInputMode(ESurvivalInputMode::GameAndUI, PauseWidget, true);
	}

	ToggleWidget(PauseWidget);
}

void ASurvivalHUD::HandleGameOverToggle()
{
	const bool bIsVisible = GameOverWidget->GetVisibility() == ESlateVisibility::Visible;

	if (bIsVisible)
	{
		SetInputMode(ESurvivalInputMode::GameOnly, nullptr, false);
	}
	else
	{
		GameOverWidget->HandleGameOver();
		SetInputMode(ESurvivalInputMode::UIOnly, GameOverWidget, true);
	}

	ToggleWidget(GameOverWidget);
}

void ASurvivalHUD::HandleGuideToggle()
{
	const bool bIsVisible = InGameWidget->GetVisibility() == ESlateVisibility::Visible;
	if (bIsVisible)
	{
		InGameWidget->ShowGuideWidget(10);
	}
}

void ASurvivalHUD::ToggleInGameHUD()
{
	if (!InGameWidget || !InventoryContainerWidget) return;
	
	const bool bIsVisible = InGameWidget->GetVisibility() == ESlateVisibility::HitTestInvisible;
	if (bIsVisible)
	{
		InGameWidget->SetVisibility(ESlateVisibility::Hidden);
		InventoryContainerWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		InGameWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		InventoryContainerWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void ASurvivalHUD::ToggleWidget(UUserWidget* Widget)
{
	if (!Widget)
		return;

	const bool bIsVisible = Widget->GetVisibility() == ESlateVisibility::Visible;

	if (bIsVisible)
	{
		Widget->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		Widget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ASurvivalHUD::SetInputMode(ESurvivalInputMode Mode, UUserWidget* FocusWidget, bool bShowCursor, bool bLockMouse)
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC) return;

	switch (Mode)
	{
	case ESurvivalInputMode::GameOnly:
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		break;
	}

	case ESurvivalInputMode::GameAndUI:
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(
			bLockMouse ? EMouseLockMode::LockAlways : EMouseLockMode::DoNotLock
		);

		if (FocusWidget)
		{
			InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
		}

		PC->SetInputMode(InputMode);
		break;
	}

	case ESurvivalInputMode::UIOnly:
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(
			bLockMouse ? EMouseLockMode::LockAlways : EMouseLockMode::DoNotLock
		);

		if (FocusWidget)
		{
			if (!FocusWidget->IsFocusable())
			{
				FocusWidget->SetIsFocusable(true);
			}

			InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
		}

		PC->SetInputMode(InputMode);
		break;
	}
	}

	PC->bShowMouseCursor = bShowCursor;
}

void ASurvivalHUD::ShowWidgetOnly(UUserWidget* TargetWidget)
{
	TArray<UUserWidget*> AllWidgets = { InGameWidget, PauseWidget, GameOverWidget, EndingWidget, InventoryContainerWidget };

	for (UUserWidget* Widget : AllWidgets)
	{
		if (!Widget) continue;

		if (TargetWidget == nullptr || Widget != TargetWidget)
		{
			Widget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (TargetWidget && TargetWidget->GetVisibility() != ESlateVisibility::Visible)
	{
		TargetWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ASurvivalHUD::HideHUD()
{
	ShowWidgetOnly(nullptr);
}

void ASurvivalHUD::OnlyShowDefaults()
{
	ShowWidgetOnly(InGameWidget);
}

