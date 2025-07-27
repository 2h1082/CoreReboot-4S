#include "Game/CheatManager/AnimalCheatHelper.h"
#include "CR4S.h"
#include "FriendlyAI/BaseAnimal.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

void UAnimalCheatHelper::KillAllAnimals()
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ABaseAnimal> ActorItr(World); ActorItr; ++ActorItr)
		{
			ABaseAnimal* Animal = *ActorItr;
			if (Animal && Animal->CurrentState != EAnimalState::Dead)
			{
				Animal->SetAnimalState(EAnimalState::Dead);
				Animal->Die();
			}
		}
		CR4S_Log(LogAnimal, Warning, TEXT("Killed all animals"));
	}
}

void UAnimalCheatHelper::StunAllAnimals(float StunDuration)
{
	if (UWorld* World = GetWorld())
	{
		int32 StunnedCount = 0;
		for (TActorIterator<ABaseAnimal> ActorItr(World); ActorItr; ++ActorItr)
		{
			ABaseAnimal* Animal = *ActorItr;
			if (Animal && Animal->CurrentState != EAnimalState::Dead && !Animal->bIsStunned)
			{
				Animal->ApplyStun(Animal->GetCurrentStats().StunThreshold + 1.0f);
				
				Animal->GetWorldTimerManager().ClearTimer(Animal->StunRecoverTimer);
				Animal->GetWorldTimerManager().SetTimer(
					Animal->StunRecoverTimer, 
					Animal, 
					&ABaseAnimal::RecoverFromStun, 
					StunDuration, 
					false
				);
                
				StunnedCount++;
			}
		}
		CR4S_Log(LogAnimal, Warning, TEXT("Stunned %d animals for %f seconds"), StunnedCount, StunDuration);
	}
}

void UAnimalCheatHelper::SpawnAnimal(const FString& AnimalName, int32 Count)
{
    if (UWorld* World = GetWorld())
    {
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
        if (!PlayerPawn)
        {
            CR4S_Log(LogAnimal, Warning, TEXT("Player not found"));
            return;
        }
    	
        FString* BPPath = AnimalBPMap.Find(AnimalName);
        if (!BPPath)
        {
            CR4S_Log(LogAnimal, Warning, TEXT("Unknown animal name: %s"), *AnimalName);
            return;
        }
    	
        UClass* AnimalClass = LoadClass<ABaseAnimal>(nullptr, **BPPath);
        if (!AnimalClass)
        {
            CR4S_Log(LogAnimal, Error, TEXT("Failed to load animal BP: %s"), **BPPath);
            return;
        }
    	
        FVector PlayerLocation = PlayerPawn->GetActorLocation();
        FVector PlayerForward = PlayerPawn->GetActorForwardVector();
        FRotator SpawnRotation = PlayerPawn->GetActorRotation();

        int32 SpawnedCount = 0;
    	
        for (int32 i = 0; i < Count; ++i)
        {
            FVector SpawnLocation = PlayerLocation + (PlayerForward * 300.0f);
            SpawnLocation += FVector(FMath::RandRange(-100.0f, 100.0f), FMath::RandRange(-100.0f, 100.0f), 0);
        	
            FHitResult HitResult;
            FVector TraceStart = SpawnLocation + FVector(0, 0, 1000);
            FVector TraceEnd = SpawnLocation - FVector(0, 0, 1000);
            
            if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic))
            {
                SpawnLocation.Z = HitResult.Location.Z;
            }
        	
            ABaseAnimal* SpawnedAnimal = World->SpawnActor<ABaseAnimal>(AnimalClass, SpawnLocation, SpawnRotation);
            
            if (SpawnedAnimal)
            {
                SpawnedCount++;
            }
        }
        
        CR4S_Log(LogAnimal, Warning, TEXT("Spawned %d %s(s)"), SpawnedCount, *AnimalName);
    }
}

void UAnimalCheatHelper::SpawnAnimalMonster(const FString& MonsterName, int32 Count)
{
    if (UWorld* World = GetWorld())
    {
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
        if (!PlayerPawn)
        {
            CR4S_Log(LogAnimal, Warning, TEXT("Player not found"));
            return;
        }
    	
        FString* BPPath = MonsterBPMap.Find(MonsterName);
        if (!BPPath)
        {
            CR4S_Log(LogAnimal, Warning, TEXT("Unknown AnimalMonster name: %s"), *MonsterName);
            return;
        }
    	
        UClass* AnimalClass = LoadClass<ABaseAnimal>(nullptr, **BPPath);
        if (!AnimalClass)
        {
            CR4S_Log(LogAnimal, Error, TEXT("Failed to load AnimalMonster BP: %s"), **BPPath);
            return;
        }
    	
        FVector PlayerLocation = PlayerPawn->GetActorLocation();
        FVector PlayerForward = PlayerPawn->GetActorForwardVector();
        FRotator SpawnRotation = PlayerPawn->GetActorRotation();

        int32 SpawnedCount = 0;
    	
        for (int32 i = 0; i < Count; ++i)
        {
            FVector SpawnLocation = PlayerLocation + (PlayerForward * 300.0f);
            SpawnLocation += FVector(FMath::RandRange(-100.0f, 100.0f), FMath::RandRange(-100.0f, 100.0f), 0);
        	
            FHitResult HitResult;
            FVector TraceStart = SpawnLocation + FVector(0, 0, 1000);
            FVector TraceEnd = SpawnLocation - FVector(0, 0, 1000);
            
            if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic))
            {
                SpawnLocation.Z = HitResult.Location.Z;
            }
        	
            ABaseAnimal* SpawnedAnimal = World->SpawnActor<ABaseAnimal>(AnimalClass, SpawnLocation, SpawnRotation);
            
            if (SpawnedAnimal)
            {
                SpawnedCount++;
            }
        }
        
        CR4S_Log(LogAnimal, Warning, TEXT("Spawned %d %s(s)"), SpawnedCount, *MonsterName);
    }
}