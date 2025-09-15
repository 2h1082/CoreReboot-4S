#pragma once

#include "CoreMinimal.h"
#include "MonsterAI/Region/RegionBossMonster.h"
#include "KamishForestBoss.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class AWeaponActor;

UCLASS()
class CR4S_API AKamishForestBoss : public ARegionBossMonster
{
	GENERATED_BODY()

public:
	AKamishForestBoss();

protected:
	virtual void BeginPlay() override;
	virtual void OnMonsterStateChanged(EMonsterState Previous, EMonsterState Current) override;
	virtual void HandleDeath() override;

	void AttachWeaponActor();
	void SpawnCloudEffect();
	void DestroyActiveClouds();
	
public:
	void SetWeaponLandingLocation(FVector Location);

	UPROPERTY()
	TArray<TObjectPtr<AWeaponActor>> WeaponActors;

	UPROPERTY()
	TObjectPtr<AWeaponActor> ThrownWeapon = nullptr;

	UPROPERTY()
	TObjectPtr<AWeaponActor> VisualWeaponActor = nullptr;

	FVector WeaponLandingLocation;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Kamish|Weapon")
	TSubclassOf<AActor> WeaponActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Kamish|Cloud")
	TObjectPtr<UNiagaraSystem> CloudAsset;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActiveCloudEffect;

	UPROPERTY(EditDefaultsOnly)
	float CloudVerticalOffset = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Kamish|Cloud")
	float CloudVisualRadius = 4000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Kamish|Cloud")
	float NiagaraRadiusBase = 100.0f;

};
