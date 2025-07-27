#include "AnimalOptimizationManager.h"
#include "FriendlyAI/BaseAnimal.h"
#include "FriendlyAI/Animation/AnimalAnimInstance.h"
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

void UAnimalOptimizationManager::ApplyDistanceBasedOptimization(ABaseAnimal* Animal, const FVector& Location)
{
    if (!Animal) return;
    
    int32 Distance = GetGridDistanceFromPlayer(Location);
    
    int32* LastDistance = LastDistanceCache.Find(Animal);
    if (!LastDistance || *LastDistance != Distance)
    {
        LastDistanceCache.Add(Animal, Distance);
        
        if (USkeletalMeshComponent* MeshComp = Animal->GetMesh())
        {
            if (Distance >= 2)
            {
                MeshComp->SetComponentTickInterval(1.0 / 10.f);
            }
            else if (Distance >= 1)
            {
                MeshComp->SetComponentTickInterval(1.0 / 2.0f);
            }
            else
            {
                MeshComp->SetComponentTickInterval(0.0f);
            }
        }
    }
}