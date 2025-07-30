#include "AnimalAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Touch.h"
#include "../BaseAnimal.h"
#include "../AnimalGround.h"
#include "../AnimalFlying.h"
#include "Character/Characters/PlayerCharacter.h"
#include "../Component/AIJumpComponent.h"
#include "Character/Characters/ModularRobot.h"
#include "FriendlyAI/Component/FlyingMovementComponent.h"
#include "Components/SphereComponent.h"
#include "FriendlyAI/AnimalMonster.h"
#include "Kismet/GameplayStatics.h"
#include "MonsterAI/BaseMonster.h"

AAnimalAIController::AAnimalAIController()
{
    PrimaryActorTick.bCanEverTick = false;
    
    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComp"));
    BlackboardComponent   = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
    AIPerceptionComp      = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComp"));

    SightConfig   = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));

    SightConfig->DetectionByAffiliation.bDetectEnemies   = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    HearingConfig->DetectionByAffiliation.bDetectEnemies   = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals  = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

    AIPerceptionComp->ConfigureSense(*SightConfig);
    AIPerceptionComp->ConfigureSense(*HearingConfig);
}

void AAnimalAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (ABaseAnimal* Animal = Cast<ABaseAnimal>(InPawn))
    {
        ApplyPerceptionStats(Animal->GetCurrentStats());
        AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
            this, &AAnimalAIController::OnTargetPerceptionUpdated
        );

        Animal->OnSpawnableDestroyed.AddUObject(this, &AAnimalAIController::OnTargetDied);
        
        UBehaviorTree* SelectedBT = nullptr;
        if (Cast<AAnimalGround>(Animal))
        {
            SelectedBT = GroundBehaviorTree;
        }
        else if (Cast<AAnimalFlying>(Animal))
        {
            SelectedBT = FlyingBehaviorTree;
        }
        
        // TODO : BossMonster BehaviorTree

        // if (SelectedBT && BlackboardComponent && BehaviorTreeComponent)
        // {
        //     BlackboardComponent->InitializeBlackboard(
        //         *SelectedBT->BlackboardAsset);
        //     SetAnimalState(EAnimalState::Patrol);
        //     BehaviorTreeComponent->StartTree(*SelectedBT);
        //     
        // }
        if (SelectedBT && BlackboardComponent && BehaviorTreeComponent)
        {
            BlackboardComponent->InitializeBlackboard(
                *SelectedBT->BlackboardAsset);
            SetAnimalState(EAnimalState::Patrol);
            BehaviorTreeComponent->StartTree(*SelectedBT);

            GetWorld()->GetTimerManager().SetTimer(
                AIUpdateTimerHandle,
                this,
                &AAnimalAIController::UpdateAILogic,
                0.5f,
                true
            );
        }
    }
}

void AAnimalAIController::SetAITimerInterval(float Interval)
{
    GetWorld()->GetTimerManager().ClearTimer(AIUpdateTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        AIUpdateTimerHandle,
        this,
        &AAnimalAIController::UpdateAILogic,
        Interval,
        true
    );
}

void AAnimalAIController::ApplyPerceptionStats(const FAnimalStatsRow& Stats)
{
    SightConfig->SightRadius                  = Stats.SightRadius;
    SightConfig->PeripheralVisionAngleDegrees = Stats.PeripheralVisionAngle;
    HearingConfig->HearingRange               = Stats.HearingRadius;

    AIPerceptionComp->ConfigureSense(*SightConfig);
    AIPerceptionComp->ConfigureSense(*HearingConfig);
    AIPerceptionComp->RequestStimuliListenerUpdate();
}

void AAnimalAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (ABaseAnimal* Animal = Cast<ABaseAnimal>(GetPawn()))
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            if (Animal->BehaviorTypeEnum == EAnimalBehavior::Aggressive)
            {
                if (Cast<AAnimalMonster>(Actor) || Cast<ABaseMonster>(Actor))
                {
                    return;
                }
            }

            if (ABaseAnimal* SensedAnimal = Cast<ABaseAnimal>(Actor))
            {
                if (SensedAnimal->CurrentState == EAnimalState::Dead ||
                    SensedAnimal->CurrentState == EAnimalState::Stun)
                {
                    return;
                }

                if (SensedAnimal->RowName == Animal->RowName)
                {
                    return;
                }
            }
            
            HandlePerceptionResponse(Animal, Actor);
        }
        else
        {
            if (IsValid(Animal->CurrentTarget))
            {
                float Distance = FVector::Dist(
                    Animal->GetActorLocation(),
                    Animal->CurrentTarget->GetActorLocation()
                );

                if (const FAnimalStatsRow* Stats = Animal->GetStatsRowPtr())
                {
                    if (Distance >= Stats->TargetLostRange)
                    {
                        BlackboardComponent->ClearValue(TEXT("TargetActor"));
                        Animal->CurrentTarget = nullptr;
                    }
                }
            }
        }
    }
}

void AAnimalAIController::HandlePerceptionResponse(ABaseAnimal* Animal, AActor* SensedActor)
{
    if (!Animal || !Animal->bStatsReady) return;

    switch (Animal->BehaviorTypeEnum)
    {
    case EAnimalBehavior::Aggressive:
        if (Cast<APlayerCharacter>(SensedActor))
        {
            if (Animal->bPlayerDead)
            {
                return;
            }
            AActor* PlayerTarget = GetCurrentPlayerTarget();
            SetTargetActor(PlayerTarget);
        }
        else if (AModularRobot* SensedRobot = Cast<AModularRobot>(SensedActor))
        {
            if (SensedRobot->GetMountedCharacter())
            {
                SetTargetActor(SensedRobot);
            }
        }
        else
        {
            SetTargetActor(SensedActor);
        }
        SetAnimalState(EAnimalState::Chase);
        break;

    case EAnimalBehavior::Coward:
        if (ABaseAnimal* TargetAnimal = Cast<ABaseAnimal>(SensedActor))
        {
            if (TargetAnimal->BehaviorTypeEnum == EAnimalBehavior::Aggressive)
            {
                SetTargetActor(SensedActor);
                SetAnimalState(EAnimalState::Flee);
            }
        }
        else if (Cast<APlayerCharacter>(SensedActor) || Cast<AModularRobot>(SensedActor))
        {
            if (Cast<APlayerCharacter>(SensedActor))
            {
                if (Animal->bPlayerDead)
                {
                    return;
                }
                AActor* PlayerTarget = GetCurrentPlayerTarget();
                SetTargetActor(PlayerTarget);
            }
            else if (AModularRobot* SensedRobot = Cast<AModularRobot>(SensedActor))
            {
                if (SensedRobot->GetMountedCharacter())
                {
                    SetTargetActor(SensedRobot);
                }
            }
            SetAnimalState(EAnimalState::Flee);
        }
        break;

    case EAnimalBehavior::Passive_AggroOnHit:
        break;
        
    case EAnimalBehavior::Passive_FleeOnHit:
        break;

    default:
        break;
    }
}

void AAnimalAIController::UpdateAILogic()
{
    ABaseAnimal* Animal = Cast<ABaseAnimal>(GetPawn());
    if (!Animal || !Animal->bStatsReady || Animal->CurrentState == EAnimalState::Dead || Animal->CurrentState == EAnimalState::Stun)
    {
        return;
    }
    
    UObject* BBTarget = BlackboardComponent->GetValueAsObject(TEXT("TargetActor"));
    if (!IsValid(BBTarget) && !IsValid(Animal->CurrentTarget))
    {
        ClearTargetActor();
        if (Animal->CurrentState != EAnimalState::Patrol)
        {
            SetAnimalState(EAnimalState::Patrol);
        }
        return;
    }
    
    if (!IsValid(BBTarget) && Animal->CurrentState == EAnimalState::Chase)
    {
        //UE_LOG(LogTemp, Warning, TEXT("[Animal] Chase : Blackboard TargetActor null"));
    }
    
    const FAnimalStatsRow* Stats = Animal->GetStatsRowPtr();
    if (!Stats) return;

    if (!IsValid(Animal->CurrentTarget))
    {
        ClearTargetActor();
        if (Animal->CurrentState != EAnimalState::Patrol)
        {
            SetAnimalState(EAnimalState::Patrol);
        }
        return;
    }

    float Distance = FVector::Dist(
        Animal->GetActorLocation(),
        Animal->CurrentTarget->GetActorLocation()
    );
    
    if (ABaseAnimal* TargetAnimal = Cast<ABaseAnimal>(Animal->CurrentTarget))
    {
        if (TargetAnimal->CurrentState == EAnimalState::Dead)
        {
            ClearTargetActor();
            if (Animal->CurrentState != EAnimalState::Patrol)
            {
                SetAnimalState(EAnimalState::Patrol);
            }
            return;
        }
    }
    
    if (Animal->BehaviorTypeEnum == EAnimalBehavior::Aggressive ||
    Animal->BehaviorTypeEnum == EAnimalBehavior::Passive_AggroOnHit)
    {
        if (Animal->CurrentState == EAnimalState::Chase)
        {
            // const float AttackDistance = FVector::Dist(Animal->GetActorLocation(), Animal->CurrentTarget->GetActorLocation());
            // if (AttackDistance > 50.f)
            // {
            //     SetAnimalState(EAnimalState::Attack);
            //     return;
            // }
    
            if (Distance >= Stats->TargetLostRange)
            {
                ClearTargetActor();
                if (Animal->CurrentState != EAnimalState::Patrol)
                {
                    SetAnimalState(EAnimalState::Patrol);
                }
                return;
            }
        }
    
        if (Animal->CurrentState == EAnimalState::Attack)
        {
            const bool bTargetDead =
                Cast<ABaseAnimal>(Animal->CurrentTarget) &&
                Cast<ABaseAnimal>(Animal->CurrentTarget)->CurrentState == EAnimalState::Dead;
            
            float MaxAttackRange = 0.f;
            if (Animal->bCanRanged)
            {
                MaxAttackRange = Animal->RangedRange;
            }
            else if (Animal->bCanCharge)
            {
                MaxAttackRange = Animal->DashRange;
            }
            else if (Animal->bCanMelee)
            {
                MaxAttackRange = Animal->MeleeRange;
            }
    
            const bool bOutOfAttackRange = Distance > MaxAttackRange;
    
            if (!IsValid(Animal->CurrentTarget) || bTargetDead || Distance >= Stats->TargetLostRange)
            {
                ClearTargetActor();
                if (Animal->BehaviorTypeEnum == EAnimalBehavior::Aggressive)
                {
                    SetAnimalState(EAnimalState::Chase);
                }
                else
                {
                    if (Animal->CurrentState != EAnimalState::Patrol)
                    {
                        SetAnimalState(EAnimalState::Patrol);
                    }
                }
                return;
            }
    
            if (bOutOfAttackRange && Animal->CurrentState != EAnimalState::Chase)
            {
                SetAnimalState(EAnimalState::Chase);
                return;
            }
        }
    }
    
    if (Animal->BehaviorTypeEnum == EAnimalBehavior::Coward && Animal->CurrentState == EAnimalState::Flee)
    {
        if (Distance >= Stats->TargetLostRange)
        {
            ClearTargetActor();
            if (Animal->CurrentState != EAnimalState::Patrol)
            {
                SetAnimalState(EAnimalState::Patrol);
            }
            return;
        }
    }
    
    if (Animal && Animal->bStatsReady && Animal->CurrentState != EAnimalState::Dead)
    {
        // const float Radius = Animal->GetCurrentStats().TargetLostRange;
        // const FVector Center = Animal->GetActorLocation();
        //
        // DrawDebugSphere(
        //     GetWorld(),
        //     Center,
        //     Radius,
        //     24,
        //     FColor::Green,
        //     false,
        //     0.1f,
        //     0,
        //     1.5f
        // );
    }
    // DrawDebugLine(Animal->GetWorld(),
    //           Animal->GetActorLocation(),
    //           Animal->CurrentTarget->GetActorLocation(),
    //           FColor::Red,
    //           false,
    //           0.f,
    //           0,
    //           2.f);
}

void AAnimalAIController::SetAnimalState(EAnimalState NewState)
{
    if (ABaseAnimal* Animal = Cast<ABaseAnimal>(GetPawn()))
    {
        Animal->SetAnimalState(NewState);

        // if (AAnimalGround* GroundAnimal = Cast<AAnimalGround>(Animal))
        // {
        //     if (GroundAnimal->AIJumpComponent && GroundAnimal->AIJumpComponent->IsActive())
        //     {
        //         GroundAnimal->AIJumpComponent->DeactivateJumpComponent();
        //     }
        // }
    }
}

void AAnimalAIController::SetTargetByDamage(AActor* Attacker)
{
    if (!IsValid(Attacker)) return;

    if (ABaseAnimal* Animal = Cast<ABaseAnimal>(GetPawn()))
    {
        const FAnimalStatsRow* Stats = Animal->GetStatsRowPtr();
        if (!Stats) return;

        const float DistanceToNew = FVector::Dist(Animal->GetActorLocation(), Attacker->GetActorLocation());
        const float DistanceToCurrent = IsValid(Animal->CurrentTarget)
            ? FVector::Dist(Animal->GetActorLocation(), Animal->CurrentTarget->GetActorLocation())
            : FLT_MAX;

        const bool bShouldChangeTarget = !IsValid(Animal->CurrentTarget) ||
        ((Animal->BehaviorTypeEnum == EAnimalBehavior::Aggressive ||
            Animal->BehaviorTypeEnum == EAnimalBehavior::Coward) && DistanceToNew < DistanceToCurrent);
        
        if (bShouldChangeTarget || !IsValid(Animal->CurrentTarget) || DistanceToNew < DistanceToCurrent)
        {
            if (Cast<APlayerCharacter>(Attacker) || Cast<AModularRobot>(Attacker))
            {
                AActor* PlayerTarget = GetCurrentPlayerTarget();
                if (IsValid(PlayerTarget))
                {
                    SetTargetActor(PlayerTarget);
                }
            }
            else
            {
                SetTargetActor(Attacker);
            }

            switch (Animal->BehaviorTypeEnum)
            {
            case EAnimalBehavior::Aggressive:
            case EAnimalBehavior::Passive_AggroOnHit:
                SetAnimalState(EAnimalState::Chase);
                break;

            case EAnimalBehavior::Coward:
            case EAnimalBehavior::Passive_FleeOnHit:
                SetAnimalState(EAnimalState::Flee);
                break;

            default:
                break;
            }
        }
    }
}

void AAnimalAIController::OnTargetDied(AActor* DeadActor)
{
    if (ABaseAnimal* Animal = Cast<ABaseAnimal>(GetPawn()))
    {
        if (Animal->CurrentTarget == DeadActor)
        {
            ClearTargetActor();
            SetAnimalState(EAnimalState::Patrol);
        }
    }
}

void AAnimalAIController::OnTargetOutOfRange()
{
    SetAnimalState(EAnimalState::Chase);
}

void AAnimalAIController::OnStunned()
{
    SetAnimalState(EAnimalState::Stun);
}

void AAnimalAIController::OnRecoveredFromStun()
{
    SetAnimalState(EAnimalState::Patrol);
}

void AAnimalAIController::OnDied()
{
    SetAnimalState(EAnimalState::Dead);
}

void AAnimalAIController::SetTargetActor(AActor* Target)
{
    if (ABaseAnimal* Animal = Cast<ABaseAnimal>(GetPawn()))
    {
        if (Animal->CurrentTarget == Target) return;

        Animal->CurrentTarget = Target;
        if (BlackboardComponent)
        {
            BlackboardComponent->SetValueAsObject(TEXT("TargetActor"), Target);
        }
    }
}

void AAnimalAIController::ClearTargetActor()
{
    if (ABaseAnimal* Animal = Cast<ABaseAnimal>(GetPawn()))
    {
        Animal->CurrentTarget = nullptr;
        if (BlackboardComponent)
        {
            BlackboardComponent->ClearValue(TEXT("TargetActor"));
        }
    }
}

AActor* AAnimalAIController::GetCurrentPlayerTarget() const
{
    APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
    if (!IsValid(Player)) return nullptr;

    if (ABaseAnimal* Animal = Cast<ABaseAnimal>(GetPawn()))
    {
        if (Animal->bPlayerDead) return nullptr;
    }
    
    TArray<AActor*> FoundRobots;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AModularRobot::StaticClass(), FoundRobots);

    for (AActor* RobotActor : FoundRobots)
    {
        if (AModularRobot* Robot = Cast<AModularRobot>(RobotActor))
        {
            APlayerCharacter* MountedChar = Robot->GetMountedCharacter();            
            if (MountedChar == Player)
            {
                return Robot;
            }
        }
    }
    return Player;
}