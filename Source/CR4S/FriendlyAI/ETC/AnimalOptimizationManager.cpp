#include "AnimalOptimizationManager.h"
#include "EngineUtils.h"
#include "NiagaraComponent.h"
#include "FriendlyAI/BaseAnimal.h"
#include "FriendlyAI/Animation/AnimalAnimInstance.h"
#include "FriendlyAI/Controller/AnimalAIController.h"
#include "Game/System/SpawnZoneManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

void UAnimalOptimizationManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    GetWorld()->GetTimerManager().SetTimer(
        UpdateTimerHandle,
        this,
        &UAnimalOptimizationManager::UpdatePlayerGridPosition,
        0.5f,
        true
    );
    
    GetWorld()->GetTimerManager().SetTimer(
        CacheTimerHandle,
        this,
        &UAnimalOptimizationManager::CacheEffectsAndNiagara,
        2.0f,
        false
    );
}

void UAnimalOptimizationManager::Deinitialize()
{
    GetWorld()->GetTimerManager().ClearTimer(UpdateTimerHandle);
    Super::Deinitialize();
}

void UAnimalOptimizationManager::UpdatePlayerGridPosition()
{
    if (!SpawnZoneManager)
    {
        SpawnZoneManager = GetWorld()->GetSubsystem<USpawnZoneManager>();
    }
    
    if (!SpawnZoneManager) return;

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn) return;

    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector2D PlayerLocation2D(PlayerLocation.X, PlayerLocation.Y);
    
    PlayerGridCoord = SpawnZoneManager->GetGridCoord(PlayerLocation2D);
    
    if (PlayerGridCoord != PreviousPlayerGridCoord)
    {
        CalculateGridDistances();
        PreviousPlayerGridCoord = PlayerGridCoord;
    }
}

void UAnimalOptimizationManager::CalculateGridDistances()
{
    GridDistanceCache.Empty();
    
    const int32 MaxRange = 10;
    
    for (int32 X = PlayerGridCoord.X - MaxRange; X <= PlayerGridCoord.X + MaxRange; ++X)
    {
        for (int32 Y = PlayerGridCoord.Y - MaxRange; Y <= PlayerGridCoord.Y + MaxRange; ++Y)
        {
            FIntPoint GridCoord(X, Y);
            int32 Distance = CalculateManhattanDistance(PlayerGridCoord, GridCoord);
            GridDistanceCache.Add(GridCoord, Distance);
        }
    }
}

int32 UAnimalOptimizationManager::CalculateManhattanDistance(const FIntPoint& GridA, const FIntPoint& GridB) const
{
    return FMath::Abs(GridA.X - GridB.X) + FMath::Abs(GridA.Y - GridB.Y);
}

int32 UAnimalOptimizationManager::GetGridDistanceFromPlayer(const FVector& Location) const
{
    if (!SpawnZoneManager) return 999;
    
    FVector2D AnimalLocation2D(Location.X, Location.Y);
    FIntPoint AnimalGrid = SpawnZoneManager->GetGridCoord(AnimalLocation2D);
    
    return CalculateManhattanDistance(PlayerGridCoord, AnimalGrid);
}

void UAnimalOptimizationManager::CacheEffectsAndNiagara()
{
    TArray<AActor*> Animals;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseAnimal::StaticClass(), Animals);

    for (AActor* Actor : Animals)
    {
        ABaseAnimal* Animal = Cast<ABaseAnimal>(Actor);
        if (!Animal || !IsValid(Animal)) continue;

        TWeakObjectPtr<ABaseAnimal> WeakAnimal = Animal;

        TArray<UParticleSystemComponent*> ParticleComps;
        Animal->GetComponents<UParticleSystemComponent>(ParticleComps);
        if (ParticleComps.Num() > 0)
        {
            TArray<TWeakObjectPtr<UParticleSystemComponent>> WeakParticleComps;
            for (UParticleSystemComponent* Comp : ParticleComps)
            {
                WeakParticleComps.Add(Comp);
            }
            CachedParticleComponents.Add(WeakAnimal, WeakParticleComps);
        }

        TArray<UNiagaraComponent*> NiagaraComps;
        Animal->GetComponents<UNiagaraComponent>(NiagaraComps);
        if (NiagaraComps.Num() > 0)
        {
            TArray<TWeakObjectPtr<UNiagaraComponent>> WeakNiagaraComps;
            for (UNiagaraComponent* Comp : NiagaraComps)
            {
                WeakNiagaraComps.Add(Comp);
            }
            CachedNiagaraComponents.Add(WeakAnimal, WeakNiagaraComps);
        }

        AnimalEffectStateCache.Add(WeakAnimal, true);
    }
}

void UAnimalOptimizationManager::ToggleEffects(ABaseAnimal* Animal, bool bEnable)
{
    if (!Animal || !IsValid(Animal)) return;
    TWeakObjectPtr<ABaseAnimal> WeakAnimal = Animal;
    
    bool* LastState = AnimalEffectStateCache.Find(WeakAnimal);
    if (LastState && *LastState == bEnable) return;
    AnimalEffectStateCache.Add(WeakAnimal, bEnable);
    
    if (TArray<TWeakObjectPtr<UParticleSystemComponent>>* ParticleComps = CachedParticleComponents.Find(WeakAnimal))
    {
        for (TWeakObjectPtr<UParticleSystemComponent>& WeakComp : *ParticleComps)
        {
            if (UParticleSystemComponent* ParticleComp = WeakComp.Get())
            {
                if (bEnable)
                {
                    ParticleComp->SetHiddenInGame(false);
                }
                else
                {
                    ParticleComp->SetHiddenInGame(true);
                }
            }
        }
    }
    
    if (TArray<TWeakObjectPtr<UNiagaraComponent>>* NiagaraComps = CachedNiagaraComponents.Find(WeakAnimal))
    {
        for (TWeakObjectPtr<UNiagaraComponent>& WeakComp : *NiagaraComps)
        {
            if (UNiagaraComponent* NiagaraComp = WeakComp.Get())
            {
                if (bEnable)
                {
                    NiagaraComp->SetHiddenInGame(false);
                }
                else
                {
                    NiagaraComp->SetHiddenInGame(true);
                }
            }
        }
    }
}

void UAnimalOptimizationManager::ApplyDistanceBasedOptimization(ABaseAnimal* Animal, const FVector& Location)
{
    if (!Animal || !IsValid(Animal)) return;
    
    int32 Distance = GetGridDistanceFromPlayer(Location);
    
    TWeakObjectPtr<ABaseAnimal> WeakAnimal = Animal;
    int32* LastDistance = LastDistanceCache.Find(WeakAnimal);
    if (!LastDistance || *LastDistance != Distance)
    {
        LastDistanceCache.Add(WeakAnimal, Distance);
        
        if (USkeletalMeshComponent* MeshComp = Animal->GetMesh())
        {
            if (AAnimalAIController* AIController = Cast<AAnimalAIController>(Animal->GetController()))
            {
                if (Distance >= 2)
                {
                    MeshComp->SetComponentTickInterval(1.0f);
                    ToggleEffects(Animal, false);
                    AIController->SetAITimerInterval(1.0f);
                }
                else
                {
                    MeshComp->SetComponentTickInterval(0.0f);
                    ToggleEffects(Animal, true);
                    AIController->SetAITimerInterval(0.5f);
                }
            }
        }
    }
}