#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatRangeVisualizer.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class CR4S_API ACombatRangeVisualizer : public AActor
{
	GENERATED_BODY()
	
public:	
	ACombatRangeVisualizer();

public:
	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void InitializeVisualizer(FVector InCenter, float Radius, float Height = 500.f);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNiagaraComponent> RangeEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat")
	TObjectPtr<UNiagaraSystem> RangeEffectSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat")
	FName RadiusParamName = FName("User_CombatRadius");

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat")
	FName HeightParamName = FName("User_WallHeight");

};
