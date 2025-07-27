#include "BTTask_AnimalAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "../Controller/AnimalAIController.h"
#include "../BaseAnimal.h"
#include "Components/SphereComponent.h"

UBTTask_AnimalAttack::UBTTask_AnimalAttack()
{
	NodeName = TEXT("Animal Attack");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_AnimalAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	StoredOwnerComp = &OwnerComp;
	ABaseAnimal* Animal = Cast<ABaseAnimal>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Animal || !Animal->MeleeAttackMontage || !Animal->GetCurrentStats().AttackInterval)
		return EBTNodeResult::Failed;

	if (!Animal->AttackRange || !Animal->CurrentTarget) return EBTNodeResult::Failed;
	
	const float Distance = FVector::Dist(
	Animal->GetActorLocation(),
	Animal->CurrentTarget->GetActorLocation()
);
	float RandValue = FMath::FRand();
	
	if (Animal->CurrentState != EAnimalState::Attack ||
		(!Animal->bCanMelee && !Animal->bCanCharge && !Animal->bCanRanged))
	{
		if (AAnimalAIController* C = Cast<AAnimalAIController>(Animal->GetController()))
		{
			C->SetAnimalState(EAnimalState::Chase);
		}
		FinishLatentTask(*StoredOwnerComp, EBTNodeResult::Failed);
		return EBTNodeResult::Failed;
	}
	
	if (Distance <= Animal->RangedRange && Animal->bCanRanged && !Animal->bIsRangedOnCooldown && RandValue < Animal->RangedProbability)
	{
		float Duration = Animal->PlayRangedAttackMontage();
		Animal->GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &UBTTask_AnimalAttack::OnAttackFinished, Duration, false);
		return EBTNodeResult::InProgress;
	}
	else if (Distance <= Animal->DashRange && Animal->bCanCharge && !Animal->bIsChargeOnCooldown && RandValue < Animal->RangedProbability + Animal->ChargeProbability)
	{
		float Duration = Animal->PlayChargeAttackMontage();
		Animal->GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &UBTTask_AnimalAttack::OnAttackFinished, Duration, false);
		return EBTNodeResult::InProgress;
	}
	else if (Distance <= Animal->MeleeRange && Animal->bCanMelee && !Animal->bIsMeleeOnCooldown && RandValue < Animal->MeleeProbability)
	{
		float Duration = Animal->PlayAttackMontage();
		Animal->GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &UBTTask_AnimalAttack::OnAttackFinished, Duration, false);
		return EBTNodeResult::InProgress;
	}
	else
	{
		if (AAnimalAIController* C = Cast<AAnimalAIController>(Animal->GetController()))
		{
			C->SetAnimalState(EAnimalState::Chase);
		}
		FinishLatentTask(*StoredOwnerComp, EBTNodeResult::Succeeded);
		return EBTNodeResult::Succeeded;
	}
	
	// FinishLatentTask(*StoredOwnerComp, EBTNodeResult::Succeeded);
	// return EBTNodeResult::Succeeded;
}

void UBTTask_AnimalAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAnimalAIController* Controller = Cast<AAnimalAIController>(OwnerComp.GetAIOwner());
	ABaseAnimal* Animal = Controller ? Cast<ABaseAnimal>(Controller->GetPawn()) : nullptr;
	if (!Animal || !IsValid(Animal->CurrentTarget) || Animal->CurrentState == EAnimalState::Stun)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const float Distance = FVector::Dist(Animal->GetActorLocation(), Animal->CurrentTarget->GetActorLocation());
	const float LostRange = Animal->GetCurrentStats().TargetLostRange;

	if (!IsValid(Animal->CurrentTarget) || Animal->CurrentState == EAnimalState::Dead || Distance > LostRange)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

void UBTTask_AnimalAttack::OnAttackFinished()
{
	if (UBehaviorTreeComponent* OwnerComp = StoredOwnerComp.Get())
	{
		if (bCanFollowUp)
		{
			if (AAnimalAIController* C = Cast<AAnimalAIController>(StoredOwnerComp->GetAIOwner()))
			{
				if (ABaseAnimal* Animal = Cast<ABaseAnimal>(C->GetPawn()))
				{
					if (Animal->CurrentState == EAnimalState::Dead) return;
					AActor* CurrentPlayerTarget = C->GetCurrentPlayerTarget();
					if (IsValid(CurrentPlayerTarget))
					{
						C->SetTargetActor(CurrentPlayerTarget);
						C->SetAnimalState(EAnimalState::Chase);
					}
					else
					{
						C->SetAnimalState(EAnimalState::Patrol);
					}
				}
			}
		}
		FinishLatentTask(*StoredOwnerComp, EBTNodeResult::Succeeded);
	}
}
