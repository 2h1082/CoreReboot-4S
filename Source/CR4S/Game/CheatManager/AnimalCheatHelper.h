#pragma once

#include "UObject/NoExportTypes.h"
#include "AnimalCheatHelper.generated.h"

UCLASS()
class CR4S_API UAnimalCheatHelper : public UObject
{
	GENERATED_BODY()
    
public:
	void KillAllAnimals();
	void StunAllAnimals(float StunDuration  = 1.0f);
	void SpawnAnimal(const FString& AnimalName, int32 Count = 1);
	void SpawnAnimalMonster(const FString& MonsterName, int32 Count = 1);

private:
	TMap<FString, FString> AnimalBPMap = {
		{"Crow", "/Game/CR4S/_Blueprint/Animal/Category/Bird/BP_AnimalFlying.BP_AnimalFlying_C"},
		{"Chicken", "/Game/CR4S/_Blueprint/Animal/Category/Chicken/BP_Chicken.BP_Chicken_C"},
		{"Cow", "/Game/CR4S/_Blueprint/Animal/Category/Cow/BP_Cow.BP_Cow_C"},
		{"Deer", "/Game/CR4S/_Blueprint/Animal/Category/Deer/BP_Deer.BP_Deer_C"},
		{"Elephant", "/Game/CR4S/_Blueprint/Animal/Category/Elephant/BP_Elephant.BP_Elephant_C"},
		{"Fox", "/Game/CR4S/_Blueprint/Animal/Category/Fox/BP_Fox.BP_Fox_C"},
		{"Ibex", "/Game/CR4S/_Blueprint/Animal/Category/Ibex/BP_Ibex.BP_Ibex_C"},
		{"Sheep", "/Game/CR4S/_Blueprint/Animal/Category/Ibex/BP_Ibex.BP_Ibex_C"},
		{"Pig", "/Game/CR4S/_Blueprint/Animal/Category/Pig/BP_Pig.BP_Pig_C"},
		{"Rabbit", "/Game/CR4S/_Blueprint/Animal/Category/Rabbit/BP_Rabbit.BP_Rabbit_C"},
		{"Wolf", "/Game/CR4S/_Blueprint/Animal/Category/Wolf/BP_Wolf.BP_Wolf_C"},
		{"Zebra", "/Game/CR4S/_Blueprint/Animal/Category/Zebra/BP_Zebra.BP_Zebra_C"},
	};

	TMap<FString, FString> MonsterBPMap = {
		{"Crocodile", "/Game/CR4S/_Blueprint/Animal/Category/Monster/Crocodile/BP_CrocodileMonster.BP_CrocodileMonster_C"},
		{"Hippo", "/Game/CR4S/_Blueprint/Animal/Category/Monster/Hippo/BP_Hippo.BP_Hippo_C"},
		{"Pig", "/Game/CR4S/_Blueprint/Animal/Category/Monster/Pig/BP_PigMonster.BP_PigMonster_C"},
		{"Wolf", "/Game/CR4S/_Blueprint/Animal/Category/Monster/Wolf/BP_WolfMonster.BP_WolfMonster_C"},
	};
};