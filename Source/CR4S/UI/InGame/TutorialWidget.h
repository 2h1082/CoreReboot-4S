#pragma once

#include "Blueprint/UserWidget.h"
#include "Game/System/TutorialManager.h"
#include "GameplayTagContainer.h"
#include "TutorialWidget.generated.h"

class UTutorialSummaryWidget;
class UTextBlock;
class UVerticalBox;

UCLASS()
class CR4S_API UTutorialWidget : public UUserWidget
{
	GENERATED_BODY()

#pragma region Overrides

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

#pragma endregion

#pragma region Child Widgets

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UTutorialSummaryWidget> TutorialSummaryWidgetClass;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StepNumber;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* StepText;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* TutorialContent;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* TutorialEndAnim;

#pragma endregion

#pragma region Tutorial Widget Updates

	UFUNCTION()
	void UpdateTutorial(const FActiveTutorial& CurrentTutorial);

	UTutorialSummaryWidget* AddObjective(const FString& Description, const FString& CountText);

	void SetStepNumber(int32 Step);
	void SetStepText(const FString& Text);
	void ClearTutorials();
	void EndTutorial();
	void PlayEndAnimationAndRemove();

	// Duration to show the tutorial end animation before clearing the widget
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	float TutorialEndDuration = 5.0f;

private:
	int32 CurrentObjectiveSetID = INDEX_NONE;

	TMap<FGameplayTag, UTutorialSummaryWidget*> ObjectiveWidgets;

#pragma endregion
};
