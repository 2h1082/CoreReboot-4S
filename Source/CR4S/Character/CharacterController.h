// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CharacterController.generated.h"

/**
 * 
 */
class UInputMappingContext;
class UInputAction;
UCLASS()
class CR4S_API ACharacterController : public APlayerController
{
	GENERATED_BODY()
public:
	ACharacterController();
	
#pragma region Direct Key Binding Handle Function
	UFUNCTION(BlueprintCallable)
	void OnPauseRequested();

	UFUNCTION(BlueprintCallable)
	void OnHUDVisibilityChanged();
#pragma endregion
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;	

#pragma region Inputs
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> InputMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PauseAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> WorldMapAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> HUDVisibilityAction;
#pragma endregion

#pragma region Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Input")
	int32 MappingContextPriority{2};
#pragma endregion

};
