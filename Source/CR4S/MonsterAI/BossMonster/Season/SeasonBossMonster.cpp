#include "MonsterAI/BossMonster/Season/SeasonBossMonster.h"
#include "AIController.h"
#include "CR4S.h"
#include "NavigationInvokerComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Game/System/EnvironmentalModifierVolume.h"
#include "Game/System/EnvironmentManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MonsterAI/Components/MonsterAggroComponent.h"
#include "MonsterAI/Components/MonsterAnimComponent.h"
#include "MonsterAI/Components/MonsterAttributeComponent.h"
#include "MonsterAI/Components/MonsterStateComponent.h"
#include "MonsterAI/Data/MonsterAIKeyNames.h"
#include "Game/System/AudioManager.h"
#include "Sound/SoundBase.h"

ASeasonBossMonster::ASeasonBossMonster()
{
   NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
   NavInvoker->SetGenerationRadii(NavInvokerRadius, NavInvokerRemovalRadius);
   
   AggroComp = CreateDefaultSubobject<UMonsterAggroComponent>(TEXT("AggroComp"));
   AggroTargetKey.SelectedKeyName = FAIKeys::TargetActor;
}

void ASeasonBossMonster::BeginPlay()
{
   Super::BeginPlay();

   if (UMonsterStateComponent* StateComp = FindComponentByClass<UMonsterStateComponent>())
   {
      StateComp->SetPhase(EBossPhase::Normal);
      StateComp->OnPhaseChanged.AddUniqueDynamic(this, &ASeasonBossMonster::HandlePhaseChanged);
   }
   
   SpawnOpeningPattern();

   FTimerHandle _bgmTimerHandle;
   GetWorld()->GetTimerManager().SetTimer(
       _bgmTimerHandle,
       this,
       &ASeasonBossMonster::PlayBattleBGM,
       BattleBGMDelay,
       false
   );
}


float ASeasonBossMonster::TakeDamage(
   float DamageAmount,
   FDamageEvent const& DamageEvent,
   AController* EventInstigator,
   AActor* DamageCauser)
{
   float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
   
   AActor* AggroInstigator = nullptr;
   if (EventInstigator && EventInstigator->GetPawn())
   {
      AggroInstigator = EventInstigator->GetPawn();
   }
   else if (DamageCauser)
   {
      AggroInstigator = DamageCauser;
   }

   if (AggroComp && AggroInstigator)
   {
      AggroComp->AddDamageAggro(AggroInstigator, Actual);
   }
   
   return Actual;
}

void ASeasonBossMonster::HandleDeath()
{
   Super::HandleDeath();

   if (SpawnedEnvVolume && SpawnedEnvVolume->IsValidLowLevel())
   {
      SpawnedEnvVolume->Destroy();
      SpawnedEnvVolume = nullptr;
   }

   if (SpawnedNiagaraComp && SpawnedNiagaraComp->IsValidLowLevel())
   {
      SpawnedNiagaraComp->DestroyComponent();
      SpawnedNiagaraComp = nullptr;
   }
}

void ASeasonBossMonster::HandlePhaseChanged(EBossPhase NewPhase)
{
   if (AAIController* AI = Cast<AAIController>(GetController()))
   {
      if (UBlackboardComponent* BB = AI->GetBlackboardComponent())
      {
         BB->SetValueAsEnum(FSeasonBossAIKeys::CurrentPhase, static_cast<int32>(NewPhase));
      }
   }

   if (!AttributeComponent || !StateComponent) return;

   if (NewPhase == EBossPhase::Normal)
   {
      AttributeComponent->InitializeMonsterAttribute(MonsterID);
   }
   else if (NewPhase == EBossPhase::Berserk)
   {
      const float BaseSpeed = AttributeComponent->GetMonsterAttribute().MoveSpeed;
      const float SpeedMultiplier = StateComponent->GetCurrentSpeedMultiplier();

      if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
      {
         MoveComp->MaxWalkSpeed = BaseSpeed * SpeedMultiplier;
      }
   }

   CR4S_Log(LogMonster, Log, TEXT("[%s] HandlePhaseChanged : Current phase changed to %s"), *GetClass()->GetName(),
      *UEnum::GetDisplayValueAsText(NewPhase).ToString());
}

void ASeasonBossMonster::SpawnOpeningPattern()
{
   UWorld* World = GetWorld();
   if (!World) return;

   if (SpawnSFX)
   {
      if (UAudioManager* AudioMgr = GetGameInstance()->GetSubsystem<UAudioManager>())
      {
         AudioMgr->PlaySFX(
             SpawnSFX,
             GetActorLocation(),
             EConcurrencyType::Impact
         );
      }
   }
   
   if (AnimComponent && !AnimComponent->IsAnyMontagePlaying())
   {
      AnimComponent->PlayCombatMontage();
   }
   
   // if (AEnvironmentManager* EnvManager = Cast<AEnvironmentManager>(UGameplayStatics::GetActorOfClass(World, AEnvironmentManager::StaticClass())))
   // {
   //    SpawnedEnvVolume = EnvManager->SpawnEnvModVol(
   //       GetActorLocation(),
   //       EnvVolRadius,
   //       EnvVolHeight,
   //       EnvTempDelta,
   //       EnvHumidDelta,
   //       EnvVolChangeSpeed,
   //       -1.f);
   //    
   //    if (IsValid(SpawnedEnvVolume))
   //    {
   //       SpawnedEnvVolume->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
   //       SpawnedEnvVolume->SetActorRelativeLocation(FVector::ZeroVector);
   //    }
   // }

   FVector ActorLoc = GetActorLocation();
   const float TraceHeight = 5000.f;
   FHitResult Hit;
   FVector TraceStart = ActorLoc + FVector(0,0,TraceHeight);
   FVector TraceEnd = ActorLoc - FVector(0,0,TraceHeight);
   FCollisionQueryParams Params;
   Params.AddIgnoredActor(this);
   Params.bTraceComplex = false;

   GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params);

   float GroundZ = Hit.bBlockingHit ? Hit.Location.Z : ActorLoc.Z;
   FVector RelativeOffset(0.f, 0.f, GroundZ);
   
   if (UNiagaraSystem* RangeEffect = GetOpeningNiagara())
   {
      if (!IsValid(RangeEffect)) return;
      
      SpawnedNiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
          RangeEffect,
          GetMesh(),
          NAME_None,
          RelativeOffset,
          FRotator::ZeroRotator,
          EAttachLocation::KeepRelativeOffset,
          false,
          true,
          ENCPoolMethod::AutoRelease,
          true
      );

      if (SpawnedNiagaraComp)
      {
         SpawnedNiagaraComp->SetRelativeLocation(RelativeOffset);
         
         SpawnedNiagaraComp->SetRelativeScale3D(OpeningNiagaraScale);
         SpawnedNiagaraComp->Activate(true);
      }
   }
}

void ASeasonBossMonster::PlayBattleBGM()
{
   if (UAudioManager* AudioMgr = GetGameInstance()->GetSubsystem<UAudioManager>())
   {
      if (USoundBase* BGM = GetBattleBGM())
      {
         AudioMgr->PlayBGM(BGM);
      }
   }
}
