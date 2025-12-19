// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CharacterCheatHelper.generated.h"

/**
 * 
 */
UCLASS()
class CR4S_API UCharacterCheatHelper : public UObject
{
	GENERATED_BODY()

public:
	void SetInvincibleMode(const bool bInvincibleMode) const;
	
	void TP(const float X, const float Y, const float Z) const;
};
