#include "CharacterController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/CheatManager/C4CheatManager.h"
#include "UI/InGame/SurvivalHUD.h"

ACharacterController::ACharacterController()
{
	CheatClass = UC4CheatManager::StaticClass();
}

void ACharacterController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LP=GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem=LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(InputMappingContext,MappingContextPriority);
		}
	}
	
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ACharacterController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (IsValid(InputComponent))
	{
		auto* EnhancedInput{Cast<UEnhancedInputComponent>(InputComponent)};
		if (IsValid(EnhancedInput))
		{
			EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &ThisClass::OnPauseRequested);
			EnhancedInput->BindAction(HUDVisibilityAction, ETriggerEvent::Started, this, &ThisClass::OnHUDVisibilityChanged);
		}
	}
}

void ACharacterController::OnPauseRequested()
{
    UE_LOG(LogTemp, Log, TEXT("HandleTogglePauseMenu() called."));

    ASurvivalHUD* InGameHUD = Cast<ASurvivalHUD>(GetHUD());
	if (InGameHUD)
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully cast to ASurvivalHUD. Toggling pause menu."));
		InGameHUD->HandlePauseToggle();
	}
}

void ACharacterController::OnHUDVisibilityChanged()
{
	ASurvivalHUD* InGameHUD = Cast<ASurvivalHUD>(GetHUD());
	if (InGameHUD)
	{
		InGameHUD->ToggleInGameHUD();
	}
}
