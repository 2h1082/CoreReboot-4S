#include "Game/System/SeasonManager.h"
#include "Game/System/SeasonType.h"
#include "Game/System/EnvironmentManager.h"
#include "MonsterAI/Data/SeasonWave.h"
#include "MonsterAI/BossMonster/Season/SeasonBossMonster.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

bool USeasonManager::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World)
	{
		return false;
	}

	if (World->GetName() == TEXT("MenuLevel"))
	{
		return false;
	}

	if (!World->IsGameWorld())
	{
		return false;
	}

	return true;
}

void USeasonManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Warning, TEXT("SeasonManager: Initialize called"));

	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &USeasonManager::OnPostWorldInit);

	//Load Season Wave DataAsset
	const FString AssetPath = TEXT("/Game/CR4S/_Data/Monster/Season/DA_SeasonWave.DA_SeasonWave");
	USeasonWave* LoadedAsset = Cast<USeasonWave>(
		StaticLoadObject(USeasonWave::StaticClass(), nullptr, *AssetPath));

	if (LoadedAsset)
	{
		SeasonWaveData = LoadedAsset;
		UE_LOG(LogTemp, Log, TEXT("SeasonManager: Successfully loaded SeasonWaveDataAsset"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SeasonManager: Failed to load SeasonWaveDataAsset from path: %s"), *AssetPath);
	}
}

void USeasonManager::OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS)
{
	EnvironmentManager = Cast<AEnvironmentManager>(UGameplayStatics::GetActorOfClass(World, AEnvironmentManager::StaticClass()));
	if (!EnvironmentManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("SeasonManager: EnvironmentManager not found"));
	}


	GetTargetDawnDuskTimeForSeason(CurrentSeason, TargetDawnTime, TargetDuskTime);
}


void USeasonManager::Deinitialize()
{
	Super::Deinitialize();
	UE_LOG(LogTemp, Warning, TEXT("SeasonManager: Deinitialize called"));

}
void USeasonManager::LoadSeason()
{
	UE_LOG(LogTemp, Warning, TEXT("Loading Season"));

	GetTargetDawnDuskTimeForSeason(CurrentSeason, TargetDawnTime, TargetDuskTime);
}

void USeasonManager::HandleDayChange()
{
	if (!EnvironmentManager) return;

	float Progress = static_cast<float>(CurrentSeasonDay) / static_cast<float>(SeasonLength);
	CurrentDawnTime = FMath::Lerp(StartDawnTime, TargetDawnTime, Progress);
	CurrentDuskTime = FMath::Lerp(StartDuskTime, TargetDuskTime, Progress);

	EnvironmentManager->SetDayNightByTime(CurrentDawnTime, CurrentDuskTime);
	OnDayChanged.Broadcast(CurrentDawnTime, CurrentDuskTime);

	CurrentSeasonDay++;

	if (CurrentSeasonDay > SeasonLength)
	{
		ChangeToNextSeason();
		CurrentSeasonDay = 1;

		StartDawnTime = CurrentDawnTime;
		StartDuskTime = CurrentDuskTime;
	}
	else if (CurrentSeasonDay == SeasonLength)
	{
		SpawnSeasonBoss();
		SpawnSeasonMonsters();
	}
}


void USeasonManager::SetCurrentDawnDuskTime(float Dawn, float Dusk)
{
	CurrentDawnTime = Dawn;
	CurrentDuskTime = Dusk;

	OnDayChanged.Broadcast(CurrentDawnTime, CurrentDuskTime);
}

void USeasonManager::GetTargetDawnDuskTimeForSeason(ESeasonType Season, float& OutDawnTime, float& OutDuskTime)
{
	switch (Season)
	{
	case ESeasonType::BountifulSeason:
		OutDawnTime = 900.0f;
		OutDuskTime = 1500.0f;
		break;
	case ESeasonType::FrostSeason:
		OutDawnTime = 800.0f;
		OutDuskTime = 1600.0f;
		break;
	case ESeasonType::RainySeason:
		OutDawnTime = 500.0f;
		OutDuskTime = 1900.0f;
		break;
	case ESeasonType::DrySeason:
		OutDawnTime = 600.0f;
		OutDuskTime = 1600.0f;
		break;
	default:
		OutDawnTime = 600.0f;
		OutDuskTime = 1800.0f;
		break;
	}
}


void USeasonManager::ChangeToNextSeason()
{
	int32 CurrentSeasonInt = static_cast<int32>(CurrentSeason);
	int32 NextSeasonInt = (CurrentSeasonInt + 1) % (static_cast<int32>(ESeasonType::DrySeason) + 1);

	ESeasonType NextSeason = static_cast<ESeasonType>(NextSeasonInt);
	if (NextSeason == ESeasonType::None)
	{
		NextSeason = ESeasonType::BountifulSeason; // Loop back to the first season
	}

	SetCurrentSeason(NextSeason);	
}


void USeasonManager::SetCurrentSeason(ESeasonType NewSeason)
{
	CurrentSeason = NewSeason;
	UE_LOG(LogTemp, Warning, TEXT("Current Season changed to: %s"), *UEnum::GetValueAsString(NewSeason));

	EnvironmentManager->SetWeatherBySeason(NewSeason);
	OnSeasonChanged.Broadcast(NewSeason);
	GetTargetDawnDuskTimeForSeason(CurrentSeason, TargetDawnTime, TargetDuskTime);
}

TArray<FMonsterWaveEntry> USeasonManager::GetMonsterWaveEntriesForSeason(ESeasonType Season) const
{
	if (!SeasonWaveData) return {};

	const FMonsterWaveArray* WaveArray = SeasonWaveData->SeasonWaves.Find(Season);
	if (WaveArray)
	{
		return WaveArray->Entries;
	}

	return {};
}


TSubclassOf<ASeasonBossMonster> USeasonManager::GetSeasonBossClass(ESeasonType Season) const
{
	if (!SeasonWaveData) return nullptr;

	const FMonsterWaveArray* WaveArray = SeasonWaveData->SeasonWaves.Find(Season);
	if (!WaveArray) return nullptr;

	for (const FMonsterWaveEntry& Entry : WaveArray->Entries)
	{
		if (Entry.bIsBoss && Entry.MonsterClass->IsChildOf(ASeasonBossMonster::StaticClass()))
		{
			return Cast<UClass>(Entry.MonsterClass);
		}
	}

	return nullptr;
}

void USeasonManager::SpawnSeasonBoss()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TSubclassOf<ASeasonBossMonster> BossClass = GetSeasonBossClass(CurrentSeason);
	if (!BossClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SeasonManager: No boss assigned for season %s"), *UEnum::GetValueAsString(CurrentSeason));
		return;
	}

	FVector SpawnLocation = GetRandomSpawnLocationAroundPlayer(5000.f, 6000.f, 20);
	if (SpawnLocation == FVector::ZeroVector)
	{
		UE_LOG(LogTemp, Warning, TEXT("SeasonManager: Failed to find valid boss spawn location"));
		return;
	}

	if (SpawnedBoss && IsValid(SpawnedBoss))
	{
		SpawnedBoss->Destroy();
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedBoss = World->SpawnActor<ASeasonBossMonster>(BossClass, SpawnLocation, FRotator::ZeroRotator, Params);
	if (SpawnedBoss)
	{
		UE_LOG(LogTemp, Warning, TEXT("SeasonManager: Spawned boss for season %s"), *UEnum::GetValueAsString(CurrentSeason));
	}
}


void USeasonManager::SpawnSeasonMonsters()
{
	UWorld* World = GetWorld();
	if (!World || !SeasonWaveData) return;

	const TArray<FMonsterWaveEntry> WaveEntries = GetMonsterWaveEntriesForSeason(CurrentSeason);

	for (const FMonsterWaveEntry& Entry : WaveEntries)
	{
		if (Entry.bIsBoss || !Entry.MonsterClass) continue;

		FVector GroupCenter = GetRandomSpawnLocationAroundPlayer(5000.f, 6000.f, 20);
		if (GroupCenter == FVector::ZeroVector) continue;

		for (int32 i = 0; i < Entry.SpawnCount; ++i)
		{
			FVector Offset = FVector(
				FMath::RandRange(-500.f, 500.f),
				FMath::RandRange(-500.f, 500.f),
				0.f
			);
			FVector SpawnLocation = GroupCenter + Offset;

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AActor* Spawned = World->SpawnActor<AActor>(Entry.MonsterClass, SpawnLocation, FRotator::ZeroRotator, Params);
			if (Spawned)
			{
				UE_LOG(LogTemp, Log, TEXT("SeasonManager: Spawned monster %s"), *Entry.MonsterClass->GetName());
			}
		}
	}
}



FVector USeasonManager::GetRandomSpawnLocationAroundPlayer(float MinRadius, float MaxRadius, int32 MaxAttempts)
{
	UWorld* World = GetWorld();
	if (!World) return FVector::ZeroVector;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn) return FVector::ZeroVector;

	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);

	if (!NavSys) return FVector::ZeroVector;

	for (int32 i = 0; i < MaxAttempts; ++i)
	{
		const float RandAngle = FMath::RandRange(0.f, 360.f);
		const float RandRadius = FMath::RandRange(MinRadius, MaxRadius);
		const FVector Offset = FVector(FMath::Cos(FMath::DegreesToRadians(RandAngle)), FMath::Sin(FMath::DegreesToRadians(RandAngle)), 0.f) * RandRadius;
		const FVector TestLocation = PlayerLocation + Offset;

		FNavLocation OutNavLocation;
		if (NavSys->ProjectPointToNavigation(TestLocation, OutNavLocation))
		{
			return OutNavLocation.Location;
		}
	}

	return FVector::ZeroVector;
}


FString USeasonManager::GetSeasonAsString(ESeasonType Season) const
{
	switch (Season)
	{
	case ESeasonType::None:
	{
		static const FString ErrorText(TEXT("ERROR"));
		return ErrorText;
	}
	case ESeasonType::BountifulSeason:
	{
		static const FString BountifulText(TEXT("풍요기"));
		return BountifulText;
	}
	case ESeasonType::FrostSeason:
	{
		static const FString FrostText(TEXT("혹한기"));
		return FrostText;
	}
	case ESeasonType::RainySeason:
	{
		static const FString RainyText(TEXT("우기"));
		return RainyText;
	}
	case ESeasonType::DrySeason:
	{
		static const FString DryText(TEXT("건기"));
		return DryText;
	}
	default:
	{
		static const FString UnknownText(TEXT("알수없음"));
		return UnknownText;
	}
	}
}
