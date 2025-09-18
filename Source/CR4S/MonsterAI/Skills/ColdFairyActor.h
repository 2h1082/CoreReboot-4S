#pragma once

#include "CoreMinimal.h"
#include "BaseSkillActor.h"
#include "GameFramework/Actor.h"
#include "ColdFairyActor.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UObjectPoolComponent;

UCLASS()
class CR4S_API AColdFairyActor : public ABaseSkillActor
{
	GENERATED_BODY()
	
public:
	AColdFairyActor();

	UFUNCTION(BlueprintCallable, Category = "Boss|Skill")
	void InitialLaunch(AActor* InTarget, int32 InIndex, int32 InTotalCount);

protected:
	UFUNCTION()
	void OnProjectileStop(const FHitResult& ImpactResult);
	
	UFUNCTION()
	void ResetProjectile();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pool")
	TObjectPtr<UObjectPoolComponent> PoolComponent;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill")
	UProjectileMovementComponent* ProjectileMovementComp;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill")
	bool bSequentialLaunch = true;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill")
	bool bUseHoming = false;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill", meta=(EditCondition="bUseHoming"))
	float GravityScale = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill", meta=(EditCondition="bUseHoming"))
	float HomingAcceleration = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill", meta=(ClampMin="0.0"))
	float Interval = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill", meta=(ClampMin="0.0"))
	float LaunchDelay = 0.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill")
	float Speed = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill")
	float MaxSpeed = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill")
	float LifeTime = 2.f;
	
	UPROPERTY()
	TObjectPtr<AActor> TargetActor = nullptr;

	FVector LaunchDirection;

private:
	virtual void OnOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;

	void Launch();
	void HandleSequenceLaunch();
	void HandleImmediateLaunch() const;
	void StopAndStick(const FHitResult& HitResult, AActor* HitActor = nullptr, UPrimitiveComponent* HitComp = nullptr);
	
	bool bHasLaunched = false;
	int32 SpawnOrder = 0;
	int32 TotalCount = 0;

	FTimerHandle LaunchTimerHandle;
	FTimerHandle DestroyTimerHandle;
};