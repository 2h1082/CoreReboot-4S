#include "AnimalStatsSubsystem.h"
#include "UObject/ConstructorHelpers.h"

void UAnimalStatsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UObject* LoadedObject = StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/CR4S/_Data/AnimalStatsTable.AnimalStatsTable"));
	StatsDataTable = Cast<UDataTable>(LoadedObject);

	if (!StatsDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to load AnimalStatsTable via StaticLoadObject"), *LogHeader);
	}

	StatsCache.Empty();
}

const FAnimalStatsRow* UAnimalStatsSubsystem::GetStatsRow(FName RowName) const
{
	if (StatsCache.Contains(RowName))
	{
		return &StatsCache[RowName];
	}

	if (!StatsDataTable || !RowName.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Invalid StatsDataTable or RowName"), *LogHeader);
		return nullptr;
	}

	const FAnimalStatsRow* FoundRow = StatsDataTable->FindRow<FAnimalStatsRow>(RowName, TEXT("GetStatsRow"));
	if (!FoundRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Row not found for %s"), *LogHeader, *RowName.ToString());
		return nullptr;
	}

	StatsCache.Add(RowName, *FoundRow);
	return &StatsCache[RowName];
}
