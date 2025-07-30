#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "../Data/AnimalStatsRow.h"
#include "AnimalAIController.generated.h"

UCLASS()
class CR4S_API AAnimalAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAnimalAIController();
		
	UFUNCTION(BlueprintCallable)
	void SetAnimalState(EAnimalState NewState);

	UFUNCTION(BlueprintCallable)
	AActor* GetCurrentPlayerTarget() const;
	
	const FAnimalStatsRow& GetCurrentStats() const { return CurrentStats; }
	UAISenseConfig_Sight* GetSightConfig() const { return SightConfig; }
	
	void SetAITimerInterval(float Interval);
	void SetTargetByDamage(AActor* Attacker);
	void SetTargetActor(AActor* Target);
	
	void ClearTargetActor();
	void ClearAITimers() { GetWorld()->GetTimerManager().ClearTimer(AIUpdateTimerHandle); }

	void ApplyPerceptionStats(const FAnimalStatsRow& Stats);
	
	void OnTargetDied(AActor* DeadActor);
	void OnTargetOutOfRange();
	void OnStunned();
	void OnRecoveredFromStun();
	void OnDied();
	
protected:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	virtual void OnPossess(APawn* InPawn) override;
	
	void HandlePerceptionResponse(class ABaseAnimal* Animal, AActor* SensedActor);

private:
	void UpdateAILogic();

public:
	UPROPERTY(EditDefaultsOnly)
	UBehaviorTree* GroundBehaviorTree;

	UPROPERTY(EditDefaultsOnly)
	UBehaviorTree* FlyingBehaviorTree;

	UPROPERTY(EditDefaultsOnly)
	UBehaviorTree* MonsterBehaviorTree;

protected:
	UPROPERTY(VisibleAnywhere)
	UBehaviorTreeComponent* BehaviorTreeComponent;

	UPROPERTY(VisibleAnywhere)
	UBlackboardComponent* BlackboardComponent;

	UPROPERTY(VisibleAnywhere)
	UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	UAISenseConfig_Hearing* HearingConfig;
	
private:
	FTimerHandle AIUpdateTimerHandle;

	FAnimalStatsRow CurrentStats;
};