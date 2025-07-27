#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "AnimalOptimizationManager.generated.h"

class USpawnZoneManager;

UCLASS()
class CR4S_API UAnimalOptimizationManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable)
	int32 GetGridDistanceFromPlayer(const FVector& Location) const;
	void ApplyDistanceBasedOptimization(class ABaseAnimal* Animal, const FVector& Location);

private:
	void UpdatePlayerGridPosition();
	void CalculateGridDistances();
	int32 CalculateManhattanDistance(const FIntPoint& GridA, const FIntPoint& GridB) const;

private:
	UPROPERTY()
	TObjectPtr<USpawnZoneManager> SpawnZoneManager;

	TMap<ABaseAnimal*, int32> LastDistanceCache;
	
	FIntPoint PlayerGridCoord = FIntPoint(ForceInit);
	FIntPoint PreviousPlayerGridCoord = FIntPoint(ForceInit);
	
	TMap<FIntPoint, int32> GridDistanceCache;

	FTimerHandle UpdateTimerHandle;
};