#pragma once

#include "DefaultInGameWidget.h"
#include "PauseWidget.h"
#include "EndingSummaryWidget.h"
#include "GameOverWidget.h"
#include "UI/Common/LoadingWidget.h"
#include "UI/Common/NotificationWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "GameFramework/HUD.h"
#include "SurvivalHUD.generated.h"

class UInventoryContainerWidget;

enum class ESurvivalInputMode
{
	GameOnly,
	GameAndUI,
	UIOnly
};

UCLASS()
class CR4S_API ASurvivalHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UDefaultInGameWidget* GetInGameWidget() const { return InGameWidget; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UInventoryContainerWidget* GetInventoryContainerWidget() const { return InventoryContainerWidget; }

#pragma region WidgetSetting
public:

	// Adds widget to viewport with specific ZOrder and visibility
	template<typename TWidget>
	TWidget* CreateAndAddWidget(TSubclassOf<TWidget> WidgetClass, int32 ZOrder, ESlateVisibility InitialVisibility = ESlateVisibility::Collapsed)
	{
		TWidget* Widget = nullptr;
		if (WidgetClass)
		{
			Widget = CreateWidget<TWidget>(GetWorld(), WidgetClass);
			if (Widget)
			{
				Widget->AddToViewport(ZOrder);
				Widget->SetVisibility(InitialVisibility);
			}
		}
		return Widget;
	}

#pragma endregion

#pragma region WidgetClass
protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSubclassOf<UDefaultInGameWidget> InGameWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UPauseWidget> PauseWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UGameOverWidget> GameOverWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UEndingSummaryWidget> EndingSummaryWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<ULoadingWidget> LoadingWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UNotificationWidget> NotificationWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UInventoryContainerWidget> InventoryContainerWidgetClass;


#pragma endregion

#pragma region WidgetInstance
protected:
	UPROPERTY(VisibleDefaultsOnly,BlueprintReadOnly, Category="UI")
	TObjectPtr<UDefaultInGameWidget> InGameWidget;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UPauseWidget> PauseWidget;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UGameOverWidget> GameOverWidget;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UEndingSummaryWidget> EndingWidget;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UNotificationWidget> NotificationWidget;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UInventoryContainerWidget> InventoryContainerWidget;

#pragma endregion

#pragma region Utility Functions
public:
	// Toggle the visibility of InGameWidget(HUD) between visible and hidden
	void ToggleInGameHUD();
	
	// Toggles the visibility of any given widget between Visible and Collapsed.
	void ToggleWidget(UUserWidget* Widget);

	// Sets the player input mode and mouse cursor visibility.
	// Supports GameOnly, GameAndUI, and UIOnly modes with optional widget focus and mouse lock.
	void SetInputMode(ESurvivalInputMode Mode, UUserWidget* FocusWidget = nullptr, bool bShowCursor = true, bool bLockMouse = false);

	// Show Only Selected Widget
	// If TargetWidget = nullptr, HUD hides all the Child Widgets
	UFUNCTION(BlueprintCallable)
	void ShowWidgetOnly(UUserWidget* TargetWidget = nullptr);
	
	UFUNCTION(BlueprintCallable)
	void ShowMessage(const FText& InText, float Duration);

	UFUNCTION(BlueprintCallable)
	void HideHUD();

	UFUNCTION(BlueprintCallable)
	void OnlyShowDefaults ();

#pragma endregion

#pragma region WidgetHandlers
public:
	UFUNCTION(BlueprintCallable) //for prototype
	void HandlePauseToggle();
	UFUNCTION(BlueprintCallable)
	void HandleGameOverToggle();
	UFUNCTION(BlueprintCallable)
	void HandleGuideToggle();


	void ShowLoading();
	void PlayEndingSequence();
	void BindGameOverWidget(UBaseStatusComponent* BaseStatComp);



#pragma endregion

private:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
};
