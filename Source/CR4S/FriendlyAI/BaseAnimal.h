#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Game/Interface/Spawnable.h"
#include "Perception/AIPerceptionComponent.h"
#include "Engine/DataTable.h"
#include "Data/AnimalStatsRow.h"
#include "Utility/StunnableInterface.h"
#include "Game/System/AudioManager.h"
#include "Data/AnimalSoundData.h"
#include "BaseAnimal.generated.h"

class UAnimalRangedAttackComponent;
class UAnimalPerceptionComponent;
class UGroundMovementComponent;
class UNavigationInvokerComponent;
class UParticleSystemComponent;
class UAIPerceptionStimuliSourceComponent;
class UObjectPoolComponent;

UENUM(BlueprintType)
enum class EAnimalState : uint8
{
	Patrol,
	Chase,
	Attack,
	Flee,
	Perched,
	Stun,
	Dead,

	// Flying
	TakeOff,
	Cruise,
	Landing,

	// Monster
	AttackPrimaryTarget,  
	ReturnToPrimaryTarget 
};

UENUM(BlueprintType)
enum class EAnimalBehavior : uint8
{
	Aggressive,
	Coward,
	Passive_AggroOnHit,
	Passive_FleeOnHit,
	Monster
};

//DECLARE_MULTICAST_DELEGATE(FOnDied); 
//Declared in Spawnable Interface, thus Deleted 

UCLASS(Abstract)
class CR4S_API ABaseAnimal : public ACharacter, public ISpawnable, public IStunnableInterface
{
	GENERATED_BODY()

public:
	ABaseAnimal();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetActorEyesViewPoint(FVector& Location, FRotator& Rotation) const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName RowName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UAnimalSoundData* SoundData;

protected:
	void LoadStats();
	
private:
	const FAnimalStatsRow* StatsRow = nullptr;
	FAnimalStatsRow CurrentStats;

public:
	const FAnimalStatsRow* GetStatsRowPtr() const { return StatsRow; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bStatsReady = false;
	
public:	
	UPROPERTY(BlueprintReadOnly)
	float StunValue = 0.0f;
	
	UPROPERTY(BlueprintReadOnly)
	float CurrentHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float JumpPower = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	AActor* CurrentTarget = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	bool bIsFemale = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tame")
	bool bIsTamed = false;

	UPROPERTY(BlueprintReadOnly)
	EAnimalState CurrentState = EAnimalState::Patrol;
	
	UPROPERTY(BlueprintReadOnly)
	EAnimalBehavior BehaviorTypeEnum;

	UPROPERTY(BlueprintReadOnly)
	bool bPlayerDead = false;

	const FAnimalStatsRow& GetCurrentStats() const { return CurrentStats; }
	virtual void SetAnimalState(EAnimalState NewState);
	void ClearTarget();

	FOnSpawnableDestroyed OnSpawnableDestroyed;

	//Function to Return OnDied Delegate by Spawnable Interface
	virtual FOnSpawnableDestroyed* GetOnSpawnableDestroyedDelegate() override { return &OnSpawnableDestroyed; }

public:
		
	UFUNCTION(BlueprintCallable)
	virtual void ApplyStun(float Amount);

	virtual void TakeStun_Implementation(const float StunAmount) override;
	
	UFUNCTION(BlueprintCallable)
	virtual void RecoverFromStun();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	void ShowHitEffect(AActor* DamageCauser);
	
	UFUNCTION(BlueprintCallable)
	void Die(AActor* Killer);
	
	FTimerHandle StunRecoverTimer;
	
	bool bIsStunned = false;

	// void SetbIsTamed(bool bNewValue);

	// UPROPERTY(EditDefaultsOnly, Category = "UI")
	// TSubclassOf<class UAnimalInteractWidget> InteractWidgetClass;

	UFUNCTION()
	void OnInteract(AActor* Interactor);

	UPROPERTY()
	TObjectPtr<class UAnimalInteractWidget> ActiveInteractWidget;
	
	//float LastAttackTime = 0.0f;
	//float CachedAttackInterval = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects")
	TArray<class UNiagaraSystem*> HitEffectSystems;

#pragma region Attack
	
public:
	// Cooldown Reset
	void ResetMeleeCooldown() { bIsMeleeOnCooldown = false; }
	void ResetChargeCooldown() { bIsChargeOnCooldown  = false; }
	void ResetRangedCooldown() { bIsRangedOnCooldown  = false; }
	
	virtual void PerformMeleeAttack();
	virtual void PerformChargeAttack();
	virtual void PerformRangedAttack();
	
	virtual float PlayAttackMontage();
	virtual float PlayChargeAttackMontage();
	virtual float PlayRangedAttackMontage();

	virtual void ResetAttackFlag() { bIsAttacking = false; }
	void ResetMeleeAttack() { bIsMeleeOnCooldown = false; }
	void ResetChargeAttack() { bIsChargeOnCooldown = false; }
	void ResetRangedAttack() { bIsRangedOnCooldown = false; }
	
public:	
	// Attack Montages
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UAnimMontage* MeleeAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UAnimMontage* ChargeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UAnimMontage* RangedMontage;
	
	// CoolDown
	UPROPERTY(EditAnywhere, Category = "Attack Cooldown")
	float MeleeAttackCooldown = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Attack Cooldown")
	float ChargeAttackCooldown = 0.0f;
	
	UPROPERTY(EditAnywhere, Category = "Attack Cooldown")
	float RangedAttackCooldown = 0.0f;

	// CoolDown Timer
	FTimerHandle AttackTimerHandle;
	FTimerHandle MeleeAttackTimerHandle;
	FTimerHandle ChargeAttackTimerHandle;
	FTimerHandle RangedAttackTimerHandle;
	
	
	uint8 bIsAttacking : 1 = 0;
	
	// Has Attack
	uint8 bCanMelee : 1 = 0;
	uint8 bCanCharge : 1 = 0;
	uint8 bCanRanged : 1 = 0;
	
	// Cooldown State
	uint8 bIsMeleeOnCooldown : 1 = 0;
	uint8 bIsChargeOnCooldown : 1 = 0;
	uint8 bIsRangedOnCooldown : 1 = 0;

	// Attack Range
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float MeleeRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float DashRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float RangedRange = 0.0f;

	// Attack Probability
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float MeleeProbability = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ChargeProbability = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float RangedProbability = 0.0f;
	
#pragma endregion

#pragma region Attack - Target Rotation
public:
	UFUNCTION(BlueprintCallable)
	void StartRotationToTarget();
	
	UFUNCTION(BlueprintCallable)
	void StopRotationToTarget();
	
	bool IsRotatingToTarget() const { return bIsRotatingToTarget; }
	
	void UpdateRotationToTarget();

private:
	UPROPERTY()
	bool bIsRotatingToTarget = false;

	UPROPERTY()
	FRotator AttackTargetRotation;

	FTimerHandle RotationTimerHandle;
	FTimerHandle RotationDefenseTimerHandle;
#pragma endregion

#pragma region Pade Out Effect

public:
	void StartFade(bool bIsFadeIn);
	void UpdateFade(bool bIsFadeIn);
	
private:
	// Fade Out
	FTimerHandle FadeDelayTimerHandle;
	FTimerHandle FadeTimerHandle;
	float ElapsedFadeTime = 0.f;
	
#pragma endregion
	
#pragma region Debug
public:
	void DrawDebugVisuals();
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	uint8 bDrawSightDebug : 1 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	uint8 bDrawAttackRangeDebug : 1 = 0;
#pragma endregion

#pragma region SFX
	void PlayAnimalSound(const TArray<USoundBase*>& SoundArray, const FVector& Location, const EConcurrencyType SoundType, const float Pitch = 1.0f, const float StartTime = 0.0f);
#pragma endregion

#pragma region Components
	UPROPERTY(VisibleAnywhere)
	UObjectPoolComponent* ObjectPoolComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
	class USphereComponent* AttackRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
	USphereComponent* EnemyCollision;

	UPROPERTY(VisibleAnywhere)
	UGroundMovementComponent* GroundComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation")
	UNavigationInvokerComponent* NavInvokerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UInteractableComponent> InteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Attack", meta=(AllowPrivateAccess="true"))
	UAnimalRangedAttackComponent* RangedAttackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Attack", meta=(AllowPrivateAccess="true"))
	UArrowComponent* MuzzleArrow;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* StunEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Effects")
	class UNiagaraComponent* HitEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAIPerceptionStimuliSourceComponent* PerceptionStimuliSource;
#pragma endregion

#pragma region Pool
	UFUNCTION()
	void OnSpawnFromPool();

	UFUNCTION()
	void OnReturnToPool();

	private:
	UPROPERTY()
	FVector InitialMeshRelativeLocation;

	UPROPERTY()
	FRotator InitialMeshRelativeRotation;
#pragma endregion

#pragma region Tutorial
public:
	void NotifyDeathObjective(AActor* Killer);
#pragma endregion
};