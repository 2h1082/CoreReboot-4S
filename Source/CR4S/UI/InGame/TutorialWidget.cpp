#include "UI/InGame/TutorialWidget.h"
#include "UI/InGame/TutorialSummaryWidget.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

void UTutorialWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UTutorialManager* TM = GetWorld()->GetSubsystem<UTutorialManager>())
    {
        TM->OnTutorialProgressUpdated.AddDynamic(this, &UTutorialWidget::UpdateTutorial);

        UpdateTutorial(TM->GetActiveTutorial());
    }
}

void UTutorialWidget::UpdateTutorial(const FActiveTutorial& CurrentTutorial)
{
    if (!TutorialContent) return;

    if (CurrentTutorial.bTutorialEnded)
    {
        EndTutorial();
        return;
	}

    if (CurrentTutorial.Data.ObjectiveSetID == CurrentObjectiveSetID)
    {
        for (const auto& Obj : CurrentTutorial.Data.Objectives)
        {
            if (UTutorialSummaryWidget** FoundWidget = ObjectiveWidgets.Find(Obj.TutorialTag))
            {
                int32 CurrentCount = 0;
                if (CurrentTutorial.Progress.Contains(Obj.TutorialTag))
                {
                    CurrentCount = CurrentTutorial.Progress[Obj.TutorialTag];
                }

                FString CountText = FString::Printf(TEXT("%d / %d"), CurrentCount, Obj.TargetCount);
                (*FoundWidget)->SetCountText(CountText);

                if (CurrentCount >= Obj.TargetCount)
                {
                    (*FoundWidget)->PlayCompleteAnimation();
                }
            }
        }
        return;
    }

    ClearTutorials();
    ObjectiveWidgets.Empty();
    CurrentObjectiveSetID = CurrentTutorial.Data.ObjectiveSetID;

	SetStepNumber(CurrentTutorial.Data.TutorialLevel);

    for (const auto& Obj : CurrentTutorial.Data.Objectives)
    {
        int32 CurrentCount = 0;
        if (CurrentTutorial.Progress.Contains(Obj.TutorialTag))
        {
            CurrentCount = CurrentTutorial.Progress[Obj.TutorialTag];
        }

        FString CountText = FString::Printf(TEXT("%d / %d"), CurrentCount, Obj.TargetCount);

        UTutorialSummaryWidget* Widget = AddObjective(Obj.Description.ToString(), CountText);
        if (Widget)
        {
            ObjectiveWidgets.Add(Obj.TutorialTag, Widget);
        }
    }
}

UTutorialSummaryWidget* UTutorialWidget::AddObjective(const FString& Description, const FString& CountText)
{
    if (!TutorialSummaryWidgetClass || !TutorialContent)
    {
        return nullptr;
    }

    UTutorialSummaryWidget* TutorialSummaryWidget = CreateWidget<UTutorialSummaryWidget>(GetWorld(), TutorialSummaryWidgetClass);
    if (TutorialSummaryWidget)
    {
        TutorialSummaryWidget->SetVisibility(ESlateVisibility::Hidden);
        TutorialSummaryWidget->SetDescription(Description);
        TutorialSummaryWidget->SetCountText(CountText);
        TutorialContent->AddChildToVerticalBox(TutorialSummaryWidget);
        TutorialSummaryWidget->PlayCreateAnimation();
    }
    return TutorialSummaryWidget;
}

void UTutorialWidget::EndTutorial()
{
    if (!TutorialContent) return;

    ClearTutorials();

    ObjectiveWidgets.Empty();
    CurrentObjectiveSetID = -1;

	UTutorialSummaryWidget* EndWidget = CreateWidget<UTutorialSummaryWidget>(GetWorld(), TutorialSummaryWidgetClass);
    if (EndWidget)
    {
        EndWidget->SetVisibility(ESlateVisibility::Hidden);
        EndWidget->HideIcon();

        EndWidget->SetDescription(
            TEXT("모든 튜토리얼 클리어하신 것을 축하 드립니다.\n")
            TEXT("앞으로 계절을 어지럽히는 보스를 처치하고\n")
            TEXT("이 세상을 구해주세요!")
        );

        SetStepText(TEXT("완료"));
		StepNumber->SetVisibility(ESlateVisibility::Collapsed);
        EndWidget->SetCountText(TEXT(""));
        TutorialContent->AddChildToVerticalBox(EndWidget);
        EndWidget->PlayCreateAnimation();

		// Play end animation after a delay
        if (UWorld* World = GetWorld())
        {
            FTimerHandle TimerHandle;
            World->GetTimerManager().SetTimer(
                TimerHandle,
                [this]() { PlayEndAnimationAndRemove(); },
                TutorialEndDuration,
                false
            );
        }

	}
}

void UTutorialWidget::PlayEndAnimationAndRemove()
{
    if (TutorialEndAnim)
    {
        PlayAnimation(TutorialEndAnim);

        const float AnimLength = TutorialEndAnim->GetEndTime();
        if (UWorld* World = GetWorld())
        {
            FTimerHandle RemoveHandle;
            World->GetTimerManager().SetTimer(
                RemoveHandle,
                [this]() { RemoveFromParent(); },
                AnimLength,
                false
            );
        }
    }
    else
    {
        RemoveFromParent();
    }
}

void UTutorialWidget::SetStepNumber(int32 Step)
{
    if (StepNumber)
    {
        StepNumber->SetText(FText::AsNumber(Step));
	}
}

void UTutorialWidget::SetStepText(const FString& Text)
{
    if (StepText)
    {
        StepText->SetText(FText::FromString(Text));
	}
}

void UTutorialWidget::ClearTutorials()
{
    TutorialContent->ClearChildren();
}

void UTutorialWidget::NativeDestruct()
{
    Super::NativeDestruct();

    if (UWorld* World = GetWorld())
    {
        if (UTutorialManager* TM = World->GetSubsystem<UTutorialManager>())
        {
            TM->OnTutorialProgressUpdated.RemoveDynamic(this, &UTutorialWidget::UpdateTutorial);
        }
    }
    Super::NativeDestruct();
}
