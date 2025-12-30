#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatRangeVisualizer.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS()
class CR4S_API ACombatRangeVisualizer : public AActor
{
	GENERATED_BODY()

public:
	ACombatRangeVisualizer();

public:
	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void InitializeVisualizer(FVector InCenter, float Radius, float Height = 500.f);

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void ShowWall();

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void HideWall();

protected:
	virtual void BeginPlay() override;

	// Generate wall segments using SplineMesh
	void GenerateWallSegments(FVector Center, float Radius);

	// Sample ground points around the circle
	TArray<FVector> SampleGroundPoints(FVector Center, float Radius, int32 SampleCount);

#pragma region Niagara Effect (Legacy)

protected:
	UPROPERTY(VisibleAnywhere, Category = "Boss|Combat|Niagara")
	TObjectPtr<UNiagaraComponent> RangeEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Niagara")
	TObjectPtr<UNiagaraSystem> RangeEffectSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Niagara")
	FName RadiusParamName = FName("User_CombatRadius");

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Niagara")
	FName HeightParamName = FName("User_WallHeight");

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Niagara")
	bool bUseNiagaraEffect = false;

#pragma endregion

#pragma region SplineMesh Wall

protected:
	UPROPERTY(VisibleAnywhere, Category = "Boss|Combat|Wall")
	TObjectPtr<USplineComponent> WallSpline;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Wall")
	TObjectPtr<UStaticMesh> WallSegmentMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Wall")
	TObjectPtr<UMaterialInterface> WallMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Wall", meta = (ClampMin = "32", ClampMax = "256"))
	int32 SampleCount = 96;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Wall", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float GroundOffset = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Wall", meta = (ClampMin = "100.0", ClampMax = "20000.0"))
	float WallHeight = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Wall", meta = (ClampMin = "0.1", ClampMax = "100.0"))
	float WallThickness = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Wall", meta = (ClampMin = "1000.0", ClampMax = "20000.0"))
	float TraceUpDistance = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Wall", meta = (ClampMin = "1000.0", ClampMax = "20000.0"))
	float TraceDownDistance = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Wall")
	bool bUseSplineMeshWall = true;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Combat|Wall")
	bool bShowSplineDebug = true;

	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> WallSegments;

#pragma endregion

private:
	FVector CachedCenter;
	float CachedRadius;
	bool bWallGenerated = false;

};
