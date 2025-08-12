#include "Game/System/SpawnZoneVolume.h"
#include "Game/System/SpawnZoneManager.h"
#include "Game/System/SeasonManager.h"
#include "Components/SplineComponent.h"
#include "Components/SphereComponent.h"
#include "FriendlyAI/BaseAnimal.h"
#include "Game/Interface/Spawnable.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "FriendlyAI/Component/ObjectPoolComponent.h"

ASpawnZoneVolume::ASpawnZoneVolume()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
    RootComponent = SceneComponent;

    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    SplineComponent->SetupAttachment(RootComponent);

    SplineComponent->ClearSplinePoints();
    SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local);
    SplineComponent->AddSplinePoint(FVector(0, 1000, 0), ESplineCoordinateSpace::Local);
    SplineComponent->AddSplinePoint(FVector(1000, 1000, 0), ESplineCoordinateSpace::Local);
    SplineComponent->AddSplinePoint(FVector(1000, 0, 0), ESplineCoordinateSpace::Local);

    SplineComponent->SetClosedLoop(true);
}

#if WITH_EDITOR
void ASpawnZoneVolume::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

}
#endif

void ASpawnZoneVolume::BeginPlay()
{
    Super::BeginPlay();

    SpawnZoneManager = GetWorld()->GetSubsystem<USpawnZoneManager>();
    if (SpawnZoneManager)
    {
        const FBox2D Bounds2D = CalculateSplineBounds2D();
        SpawnZoneManager->RegisterZone(this, Bounds2D);
    }

    if (USeasonManager* SeasonManager = GetWorld()->GetSubsystem<USeasonManager>())
    {
        UpdateSeasonSpawns(SeasonManager->GetCurrentSeason());

        SeasonManager->OnSeasonChanged.AddDynamic(this, &ASpawnZoneVolume::OnSeasonChanged);
    }


#if WITH_EDITOR
    DrawDebugLines();
#endif
}

void ASpawnZoneVolume::SetZoneActive(bool bNewActive)
{
    if (bIsZoneActive == bNewActive)
        return;

    bIsZoneActive = bNewActive;

    if (bIsZoneActive)
    {
        SpawnActorsInZone();
    }
    else
    {
        DespawnActorsInZone();
    }
}


AActor* ASpawnZoneVolume::SpawnActorsAndTrack(TSubclassOf<AActor> SpawnClass, const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World || !SpawnClass) return nullptr;
    
    for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        if (Actor->IsA(SpawnClass) && Actor->IsHidden())
        {
            if (UObjectPoolComponent* PoolComp = Actor->FindComponentByClass<UObjectPoolComponent>())
            {
                Actor->SetActorLocation(Location);
                PoolComp->OnSpawnFromPool();
            
                if (Actor->GetClass()->ImplementsInterface(USpawnable::StaticClass()))
                {
                    ISpawnable* Spawnable = Cast<ISpawnable>(Actor);
                    if (Spawnable)
                    {
                        if (FOnSpawnableDestroyed* Delegate = Spawnable->GetOnSpawnableDestroyedDelegate())
                        {
                            Delegate->AddUObject(this, &ASpawnZoneVolume::HandleActorDeath);
                        }
                    }
                }
            
                return Actor;
            }
        }
    }
    
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    AActor* Spawned = World->SpawnActor<AActor>(SpawnClass, Location, FRotator::ZeroRotator, Params);
    if (Spawned)
    {
        if (Spawned->GetClass()->ImplementsInterface(USpawnable::StaticClass()))
        {
            ISpawnable* Spawnable = Cast<ISpawnable>(Spawned);
            if (Spawnable)
            {
                if (FOnSpawnableDestroyed* Delegate = Spawnable->GetOnSpawnableDestroyedDelegate())
                {
                    Delegate->AddUObject(this, &ASpawnZoneVolume::HandleActorDeath);
                }
            }
        }
    }
    return Spawned;
}


void ASpawnZoneVolume::SpawnActorsInZone()
{
    if (!SplineComponent || !SeasonSpawns || SeasonSpawns->Num() == 0) return;

    switch (SpawnerType)
    {
    case ESpawnerType::Creature:
        SpawnCreatures();
        break;

    case ESpawnerType::Crop:
        SpawnCrops();
        break;

    default:
        break;
    }
}


void ASpawnZoneVolume::SpawnCreatures()
{
    TArray<FVector2D> Polygon = GetPolygonFromSpline();
    if (Polygon.Num() < 3) return;

    SpawnedActors.Empty();

    for (const FSpawnClassInfo& SpawnInfo : *SeasonSpawns)
    {
        if (!SpawnInfo.SpawnClass) continue;

        if (SpawnInfo.bEnableSpawnRate)
        {
            for (int32 i = 0; i < SpawnInfo.SpawnCount; ++i)
            {
                FTimerHandle TimerHandle;
                FSpawnClassInfo SpawnInfoCopy = SpawnInfo;
                FTimerDelegate TimerDel;

                TimerDel.BindLambda([this, SpawnInfoCopy, Polygon]()
                    {
                        if (!IsValid(this)) return;

                        FVector2D RandomPoint2D;
                        if (GetRandomPointInPolygon(Polygon, RandomPoint2D))
                        {
                            FVector SpawnLocation;
                            if (!TryGetGroundSpawnLocation(RandomPoint2D, SpawnLocation))
                            {
                                SpawnLocation = FVector(RandomPoint2D.X, RandomPoint2D.Y, SpawnHeight);
                            }

                            AActor* Spawned = SpawnActorsAndTrack(SpawnInfoCopy.SpawnClass, SpawnLocation);
                            if (Spawned)
                            {
                                SpawnedActors.Add(Spawned);
                            }
                        }
                    });

                GetWorld()->GetTimerManager().SetTimer(
                    TimerHandle,
                    TimerDel,
                    FMath::Max(0.001f, i * SpawnInfo.SpawnRate),
                    false
                );

                SpawnedTimers.Add(TimerHandle);
            }
        }
        else
        {
            for (int32 i = 0; i < SpawnInfo.SpawnCount; ++i)
            {
                FVector2D RandomPoint2D;
                if (GetRandomPointInPolygon(Polygon, RandomPoint2D))
                {
                    FVector SpawnLocation;
                    if (!TryGetGroundSpawnLocation(RandomPoint2D, SpawnLocation))
                    {
                        SpawnLocation = FVector(RandomPoint2D.X, RandomPoint2D.Y, SpawnHeight);
                    }

                    AActor* Spawned = SpawnActorsAndTrack(SpawnInfo.SpawnClass, SpawnLocation);
                    if (Spawned)
                    {
                        SpawnedActors.Add(Spawned);
                    }
                }
            }
        }
    }
}



void ASpawnZoneVolume::SpawnCrops()
{
    TArray<FVector2D> Polygon = GetPolygonFromSpline();
    if (Polygon.Num() < 3) return;

    SpawnedActors.Empty();

    for (const FSpawnClassInfo& SpawnInfo : *SeasonSpawns)
    {
        if (!SpawnInfo.SpawnClass) continue;

        for (int32 i = 0; i < SpawnInfo.SpawnCount; ++i)
        {
            FVector2D RandomPoint2D;
            if (GetRandomPointInPolygon(Polygon, RandomPoint2D))
            {
                FVector SpawnLocation;
                if (!TryGetGroundSpawnLocation(RandomPoint2D, SpawnLocation))
                {
                    SpawnLocation = FVector(RandomPoint2D.X, RandomPoint2D.Y, SpawnHeight);
                }

                AActor* Spawned = SpawnActorsAndTrack(SpawnInfo.SpawnClass, SpawnLocation);
                if (Spawned)
                {
                    SpawnedActors.Add(Spawned);
                }
            }
        }
    }
}



void ASpawnZoneVolume::DespawnActorsInZone()
{
    for (int32 i = SpawnedActors.Num() - 1; i >= 0; --i)
    {
        if (AActor* Actor = SpawnedActors[i].Get())
        {
            if (UObjectPoolComponent* PoolComp = Actor->FindComponentByClass<UObjectPoolComponent>())
            {
                PoolComp->ReturnToPoolAfter(0.0f);
            }
            else
            {
                Actor->Destroy();
            }
        }
    }
    SpawnedActors.Empty();

    for (FTimerHandle& Timer : SpawnedTimers)
    {
        GetWorld()->GetTimerManager().ClearTimer(Timer);
    }
    SpawnedTimers.Empty();
}

void ASpawnZoneVolume::HandleActorDeath(AActor* Actor)
{
    if (!Actor || !SeasonSpawns) return;

    SpawnedActors.Remove(Actor);

    for (const FSpawnClassInfo& Info : *SeasonSpawns)
    {
        if (Actor->IsA(Info.SpawnClass))
        {
            if (Info.bEnableRespawn)
            {
                FTimerDelegate RespawnDelegate;
                RespawnDelegate.BindUFunction(this, FName("RespawnActor"), Info.SpawnClass);

                GetWorld()->GetTimerManager().SetTimerForNextTick([this, RespawnDelegate, Info]()
                    {
                        FTimerHandle TempHandle;
                        GetWorld()->GetTimerManager().SetTimer(
                            TempHandle,
                            RespawnDelegate,
                            Info.RespawnTime,
                            false
                        );
                    });
            }
            break;
        }
    }
}


void ASpawnZoneVolume::RespawnActor(TSubclassOf<AActor> ActorClass)
{
    if (!ActorClass || !SplineComponent || !bIsZoneActive) return;

    TArray<FVector2D> Polygon = GetPolygonFromSpline();
    FVector2D RandomPoint2D;
    if (!GetRandomPointInPolygon(Polygon, RandomPoint2D)) return;

    FVector SpawnLocation;
    if (!TryGetGroundSpawnLocation(RandomPoint2D, SpawnLocation))
    {
        SpawnLocation = FVector(RandomPoint2D.X, RandomPoint2D.Y, SpawnHeight);
    }

    AActor* NewActor = SpawnActorsAndTrack(ActorClass, SpawnLocation);
    if (NewActor)
    {
        SpawnedActors.Add(NewActor);
    }
}

void ASpawnZoneVolume::OnSeasonChanged(ESeasonType Season)
{
    UpdateSeasonSpawns(Season);

    if (bIsZoneActive)
    {
        DespawnActorsInZone();
        SpawnActorsInZone();
    }
}

void ASpawnZoneVolume::UpdateSeasonSpawns(ESeasonType Season)
{
    switch (Season)
    {
    case ESeasonType::BountifulSeason:
        SeasonSpawns = &BountifulSeasonSpawns;
        break;
    case ESeasonType::FrostSeason:
        SeasonSpawns = &FrostSeasonSpawns;
        break;
    case ESeasonType::RainySeason:
        SeasonSpawns = &RainySeasonSpawns;
        break;
    case ESeasonType::DrySeason:
        SeasonSpawns = &DrySeasonSpawns;
        break;
    default:
        SeasonSpawns = nullptr;
        break;
    }
}


bool ASpawnZoneVolume::GetRandomPointInPolygon(const TArray<FVector2D>& Polygon, FVector2D& OutPoint) const
{
    FVector2D Min(FLT_MAX), Max(-FLT_MAX);
    for (const FVector2D& P : Polygon)
    {
        Min.X = FMath::Min(Min.X, P.X);
        Min.Y = FMath::Min(Min.Y, P.Y);
        Max.X = FMath::Max(Max.X, P.X);
        Max.Y = FMath::Max(Max.Y, P.Y);
    }

    for (int32 i = 0; i < 100; ++i)
    {
        FVector2D RandPoint = FVector2D(
            FMath::FRandRange(Min.X, Max.X),
            FMath::FRandRange(Min.Y, Max.Y)
        );

        if (FPolygon2D::PointInPolygon(RandPoint, Polygon))
        {
            OutPoint = RandPoint;
            return true;
        }
    }
    return false;
}

TArray<FVector2D> ASpawnZoneVolume::GetPolygonFromSpline() const
{
    TArray<FVector2D> Polygon;

    if (!SplineComponent) return Polygon;

    const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector Point = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        Polygon.Add(FVector2D(Point.X, Point.Y));
    }

    return Polygon;
}

bool ASpawnZoneVolume::TryGetGroundSpawnLocation(const FVector2D& Point2D, FVector& OutLocation) const
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogTemp, Error, TEXT("TryGetGroundSpawnLocation: World is nullptr! Zone: %s"), *GetName());
        return false;
    }

    FVector Start(Point2D.X, Point2D.Y, 0.0f);
    FVector End(Point2D.X, Point2D.Y, -30000.f);
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.bTraceComplex = true;

    if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
    {
        OutLocation = Hit.Location + FVector(0, 0, 5.f);
        return true;
    }

    return false;
}

FBox2D ASpawnZoneVolume::CalculateSplineBounds2D() const
{
    if (!SplineComponent) return FBox2D(ForceInit);

    FBox2D Bounds2D(ForceInit);
    const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();

    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector WorldPoint = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        Bounds2D += FVector2D(WorldPoint.X, WorldPoint.Y);
    }

    return Bounds2D;
}

void ASpawnZoneVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (USeasonManager* SeasonManager = GetWorld()->GetSubsystem<USeasonManager>())
    {
        SeasonManager->OnSeasonChanged.RemoveDynamic(this, &ASpawnZoneVolume::OnSeasonChanged);
    }

    SpawnedTimers.Empty();

    Super::EndPlay(EndPlayReason);
}


#if WITH_EDITOR
void ASpawnZoneVolume::DrawDebugLines()
{
    const float Interval = 200.f;
    const float MaxDistance = SplineComponent->GetSplineLength();
    const float Duration = 0.1f;
    FVector PrevLocation;
    bool bHasPrev = false;

    for (float Distance = 0.f; Distance <= MaxDistance; Distance += Interval)
    {
        FVector Location = SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        if (bHasPrev)
        {
            DrawDebugLine(
                GetWorld(),
                PrevLocation,
                Location,
                FColor::White,
                true,
                Duration,
                0,
                10.f
            );
        }
        PrevLocation = Location;
        bHasPrev = true;
    }
}
#endif