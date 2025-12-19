// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterCheatHelper.h"

#include "Character/Components/BaseStatusComponent.h"

void UCharacterCheatHelper::SetInvincibleMode(const bool bInvincibleMode) const
{
	if (!GetWorld()) return;

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	const APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	UBaseStatusComponent* Status=Pawn->FindComponentByClass<UBaseStatusComponent>();
	if (Status)
	{
		Status->SetInvincibleMode(bInvincibleMode);
	}
}

void UCharacterCheatHelper::TP(const float X, const float Y, const float Z) const
{
	if (!GetWorld()) return;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	Pawn->TeleportTo(FVector(X, Y, Z), Pawn->GetActorRotation(), false, false);
}
