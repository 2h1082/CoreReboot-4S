#include "BTTask_AnimalChase.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "../BaseAnimal.h"
#include "Components/SphereComponent.h"
#include "../AnimalGround.h"
#include "../Component/AIJumpComponent.h"
#include "../Controller/AnimalAIController.h"

UBTTask_AnimalChase::UBTTask_AnimalChase()
{
	NodeName = TEXT("AnimalChase");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_AnimalChase::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{	
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!Controller) return EBTNodeResult::Failed;

	ABaseAnimal* Animal = Cast<ABaseAnimal>(Controller->GetPawn());
	if (!Animal) return EBTNodeResult::Failed;

	AActor* TargetActor = Cast<AActor>(
		OwnerComp.GetBlackboardComponent()->GetValueAsObject("TargetActor"));
	if (!TargetActor) return EBTNodeResult::Failed;
    
	if (ABaseAnimal* TargetAnimal = Cast<ABaseAnimal>(TargetActor))
	{
		if (Animal->CurrentState == EAnimalState::Dead || TargetAnimal->CurrentState == EAnimalState::Stun)
		{
			return EBTNodeResult::Failed;
		}
		
		if (TargetAnimal->CurrentState == EAnimalState::Dead)
		{
			if (AAnimalAIController* C = Cast<AAnimalAIController>(OwnerComp.GetAIOwner()))
			{
				if (ABaseAnimal* ControlledAnimal = Cast<ABaseAnimal>(C->GetPawn()))
				{
					if (ControlledAnimal->BehaviorTypeEnum == EAnimalBehavior::Aggressive)
					{
						C->SetAnimalState(EAnimalState::Chase);
					}
					else
					{
						C->SetAnimalState(EAnimalState::Patrol);
					}
				}
			}
			return EBTNodeResult::Failed;
		}
	}
    
	TargetToChase = TargetActor;

	const FVector SelfLocation   = Animal->GetActorLocation();
	const FVector TargetLocation = TargetToChase->GetActorLocation();
	const float Distance         = FVector::Dist(SelfLocation, TargetLocation);

	FAIMoveRequest MoveReq;
	MoveReq.SetGoalLocation(TargetLocation);
	
	float DesiredRadius = 0.f;
	if (Animal->bCanRanged && !Animal->bIsRangedOnCooldown)
	{
		DesiredRadius = Animal->RangedRange;
	}
	else if (Animal->bCanCharge && !Animal->bIsChargeOnCooldown)
	{
		DesiredRadius = Animal->DashRange;
	}
	else
	{
		DesiredRadius = Animal->MeleeRange;
	}
	MoveReq.SetAcceptanceRadius(DesiredRadius);

	// if (AAnimalGround* GroundAnimal = Cast<AAnimalGround>(Animal))
	// {
	// 	if (GroundAnimal->AIJumpComponent)
	// 	{
	// 		GroundAnimal->AIJumpComponent->ActivateJumpComponent();
	// 	}
	// }

	Controller->MoveToActor(TargetToChase, DesiredRadius);
	return EBTNodeResult::InProgress;
}


void UBTTask_AnimalChase::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{	
	ABaseAnimal* Animal = Cast<ABaseAnimal>(OwnerComp.GetAIOwner()->GetPawn());
	if (!Animal || Animal->CurrentState != EAnimalState::Chase || Animal->CurrentState == EAnimalState::Stun || Animal->CurrentState == EAnimalState::Dead)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (AAnimalAIController* AIController = Cast<AAnimalAIController>(OwnerComp.GetAIOwner()))
	{
		AActor* CurrentPlayerTarget = AIController->GetCurrentPlayerTarget();
		if (IsValid(CurrentPlayerTarget) && CurrentPlayerTarget != TargetToChase)
		{
			TargetToChase = CurrentPlayerTarget;
			Animal->CurrentTarget = CurrentPlayerTarget;
			if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
			{
				BB->SetValueAsObject(TEXT("TargetActor"), CurrentPlayerTarget);
			}
		}
	}

	ABaseAnimal* TargetAnimal = Cast<ABaseAnimal>(TargetToChase);
	if (!IsValid(TargetToChase) || (TargetAnimal && TargetAnimal->CurrentState == EAnimalState::Dead))
	{
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			BB->ClearValue(TEXT("TargetActor"));
		}
		Animal->ClearTarget();
		if (AAnimalAIController* C = Cast<AAnimalAIController>(OwnerComp.GetAIOwner()))
		{
			C->SetAnimalState(EAnimalState::Patrol);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	float DesiredRadius = 0.f;
	if (Animal->bCanRanged && !Animal->bIsRangedOnCooldown)
	{
		DesiredRadius = Animal->RangedRange;
	}
	else if (Animal->bCanCharge && !Animal->bIsChargeOnCooldown)
	{
		DesiredRadius = Animal->DashRange;
	}
	else
	{
		DesiredRadius = Animal->MeleeRange;
	}

	const float DistanceToTarget = FVector::Dist(
		Animal->GetActorLocation(),
		TargetToChase->GetActorLocation());

	// if (DistanceToTarget > DesiredRadius)
	// {
	// 	if (AAnimalGround* GroundAnimal = Cast<AAnimalGround>(Animal))
	// 	{
	// 		if (GroundAnimal->AIJumpComponent && !GroundAnimal->AIJumpComponent->GetIsComponentActive())
	// 		{
	// 			GroundAnimal->AIJumpComponent->ActivateJumpComponent(DesiredRadius);
	// 		}
	// 	}
	// }

	if (DistanceToTarget > DesiredRadius)
	{
		if (AAIController* AIController = OwnerComp.GetAIOwner())
		{
			AIController->MoveToActor(TargetToChase, 1.f);
		}
	}
	
	float Distance = FVector::Dist(
	Animal->GetActorLocation(),
	TargetToChase->GetActorLocation()
);
	
	if (Animal->bCanRanged
		&& !Animal->bIsRangedOnCooldown
		&& Distance <= Animal->RangedRange)
	{
		if (AAnimalAIController* C = Cast<AAnimalAIController>(OwnerComp.GetAIOwner()))
		{
			C->SetAnimalState(EAnimalState::Attack);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	
	if (Animal->bCanCharge
		&& !Animal->bIsChargeOnCooldown
		&& Distance <= Animal->DashRange)
	{
		if (AAnimalAIController* C = Cast<AAnimalAIController>(OwnerComp.GetAIOwner()))
		{
			C->SetAnimalState(EAnimalState::Attack);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	
	if (Animal->bCanMelee
		&& !Animal->bIsMeleeOnCooldown
		&& Distance <= Animal->MeleeRange)
	{
		if (AAnimalAIController* C = Cast<AAnimalAIController>(OwnerComp.GetAIOwner()))
		{
			C->SetAnimalState(EAnimalState::Attack);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	
	// if (Animal->bCanCharge && !Animal->bIsChargeOnCooldown && Distance <= Animal->DashRange)
	// {
	// 	if (AAnimalAIController* C = Cast<AAnimalAIController>(OwnerComp.GetAIOwner()))
	// 	{
	// 		C->SetAnimalState(EAnimalState::Attack);
	// 	}
	// 	FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	// 	return;
	// }
	//
	// if (Animal->bCanMelee && !Animal->bIsMeleeOnCooldown && Distance <= Animal->MeleeRange)
	// {
	// 	if (AAnimalAIController* C = Cast<AAnimalAIController>(OwnerComp.GetAIOwner()))
	// 	{
	// 		C->SetAnimalState(EAnimalState::Attack);
	// 	}
	// 	FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	// 	return;
	// }
}
