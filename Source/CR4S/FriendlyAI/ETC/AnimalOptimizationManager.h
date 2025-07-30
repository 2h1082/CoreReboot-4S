#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "AnimalOptimizationManager.generated.h"

class USpawnZoneManager;
class UParticleSystemComponent;
class UNiagaraComponent;

UCLASS()
class CR4S_API UAnimalOptimizationManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	int32 GetGridDistanceFromPlayer(const FVector& Location) const;
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void ApplyDistanceBasedOptimization(class ABaseAnimal* Animal, const FVector& Location);

private:
	void UpdatePlayerGridPosition();
	void CalculateGridDistances();	
	void CacheEffectsAndNiagara();
	void ToggleEffects(class ABaseAnimal* Animal, bool bEnable);
	int32 CalculateManhattanDistance(const FIntPoint& GridA, const FIntPoint& GridB) const;
	
private:
	UPROPERTY()
	TObjectPtr<USpawnZoneManager> SpawnZoneManager;

	UPROPERTY()
	TMap<FIntPoint, int32> GridDistanceCache;

	TMap<TWeakObjectPtr<ABaseAnimal>, int32> LastDistanceCache;
	TMap<TWeakObjectPtr<ABaseAnimal>, bool> AnimalEffectStateCache;
	TMap<TWeakObjectPtr<ABaseAnimal>, TArray<TWeakObjectPtr<UParticleSystemComponent>>> CachedParticleComponents;
	TMap<TWeakObjectPtr<ABaseAnimal>, TArray<TWeakObjectPtr<UNiagaraComponent>>> CachedNiagaraComponents;
	
	FIntPoint PlayerGridCoord = FIntPoint(ForceInit);
	FIntPoint PreviousPlayerGridCoord = FIntPoint(ForceInit);
	
	FTimerHandle UpdateTimerHandle;
	FTimerHandle CacheTimerHandle;
};