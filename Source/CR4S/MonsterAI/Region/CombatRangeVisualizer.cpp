#include "MonsterAI/Region/CombatRangeVisualizer.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

ACombatRangeVisualizer::ACombatRangeVisualizer()
{
	PrimaryActorTick.bCanEverTick = false;

	RangeEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RangeEffect"));
	SetRootComponent(RangeEffect);
}
void ACombatRangeVisualizer::InitializeVisualizer(FVector InCenter, float Radius, float Height)
{
	SetActorLocation(InCenter);

	if (!RangeEffect && RangeEffectSystem)
	{
		RangeEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
			RangeEffectSystem,
			GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			false,
			true,
			ENCPoolMethod::AutoRelease,
			true
		);
	}

	if (RangeEffect)
	{
		RangeEffect->SetVariableFloat(RadiusParamName, Radius / 50.f);
		RangeEffect->SetVariableFloat(HeightParamName, Height / 100.f);
	}
}
