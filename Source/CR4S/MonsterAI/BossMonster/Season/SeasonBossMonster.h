#pragma once

#include "CoreMinimal.h"
#include "MonsterAI/BaseMonster.h"
#include "MonsterAI/Task/BTTask_SelectSkill.h"
#include "SeasonBossMonster.generated.h"

class UMonsterAggroComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class AEnvironmentalModifierVolume;
class UNavigationInvokerComponent;

UCLASS()
class CR4S_API ASeasonBossMonster : public ABaseMonster
{
	GENERATED_BODY()

public:
	ASeasonBossMonster();

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;

#pragma region State Mamaging
protected:
	UFUNCTION()
	void HandlePhaseChanged(EBossPhase NewPhase);

#pragma endregion
	
#pragma region Opening Pattern
protected:
	void SpawnOpeningPattern();  // Template Method
	virtual UNiagaraSystem* GetOpeningNiagara() const PURE_VIRTUAL(ASeasonBossMonster::GetOpeningNiagara, return nullptr;)
	virtual USoundBase* GetBattleBGM() const PURE_VIRTUAL(ASeasonBossMonster::GetBattleBGM, return nullptr;)
	UFUNCTION()
	void PlayBattleBGM();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Opening Pattern")
	FVector OpeningNiagaraScale = FVector(1.f, 1.f, 1.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Opening Pattern")
	float EnvVolRadius = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Opening Pattern")
	float EnvVolHeight = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Opening Pattern")
	float EnvVolChangeSpeed = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Opening Pattern")
	float EnvTempDelta = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Opening Pattern")
	float EnvHumidDelta = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Opening Pattern")
	TObjectPtr<USoundBase> SpawnSFX;

	UPROPERTY(EditDefaultsOnly, Category="Audio")
	float BattleBGMDelay = 3.0f;
	

private:
	UPROPERTY()
	TObjectPtr<AEnvironmentalModifierVolume> SpawnedEnvVolume = nullptr;
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> SpawnedNiagaraComp = nullptr;
	
#pragma endregion

#pragma region NavInvoker
protected:
	UPROPERTY(EditAnywhere, Category = "MonsterAI")
	TObjectPtr<UNavigationInvokerComponent> NavInvoker;
	
	UPROPERTY(EditAnywhere, Category = "MonsterAI")
	float NavInvokerRadius = 5000.0f;
	UPROPERTY(EditAnywhere, Category = "MonsterAI")
	float NavInvokerRemovalRadius = 5500.f;

#pragma endregion

#pragma region Aggro
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Component")
	TObjectPtr<UMonsterAggroComponent> AggroComp;

	UPROPERTY(EditAnywhere, Category="AI")
	FBlackboardKeySelector AggroTargetKey;

	virtual float TakeDamage(float DamageAmount,
							 struct FDamageEvent const& DamageEvent,
							 AController* EventInstigator,
							 AActor* DamageCauser) override;

#pragma endregion
};
