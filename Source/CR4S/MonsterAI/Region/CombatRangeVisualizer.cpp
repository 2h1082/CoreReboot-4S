#include "MonsterAI/Region/CombatRangeVisualizer.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"

ACombatRangeVisualizer::ACombatRangeVisualizer()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Niagara effect (legacy)
	RangeEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RangeEffect"));
	RangeEffect->SetupAttachment(Root);

	// Spline for wall segments
	WallSpline = CreateDefaultSubobject<USplineComponent>(TEXT("WallSpline"));
	WallSpline->SetupAttachment(Root);
}

void ACombatRangeVisualizer::BeginPlay()
{
	Super::BeginPlay();

	if (WallSpline)
	{
		WallSpline->SetVisibility(bShowSplineDebug);
		WallSpline->bDrawDebug = bShowSplineDebug;
	}
}
void ACombatRangeVisualizer::InitializeVisualizer(FVector InCenter, float Radius, float Height)
{
	SetActorLocation(InCenter);
	CachedCenter = InCenter;
	CachedRadius = Radius;

	// Legacy Niagara effect (optional)
	if (bUseNiagaraEffect && RangeEffectSystem)
	{
		if (!RangeEffect)
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
			RangeEffect->Activate(true);
		}
	}

	// SplineMesh wall system
	if (bUseSplineMeshWall)
	{
		GenerateWallSegments(InCenter, Radius);
	}
}


void ACombatRangeVisualizer::ShowWall()
{
	for (USplineMeshComponent* Segment : WallSegments)
	{
		if (Segment)
		{
			Segment->SetVisibility(true);
		}
	}
}

void ACombatRangeVisualizer::HideWall()
{
	for (USplineMeshComponent* Segment : WallSegments)
	{
		if (Segment)
		{
			Segment->SetVisibility(false);
		}
	}
}

TArray<FVector> ACombatRangeVisualizer::SampleGroundPoints(FVector Center, float Radius, int32 Count)
{
	TArray<FVector> GroundPoints;
	GroundPoints.Reserve(Count);

	const float AngleStep = 2.0f * PI / Count;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = i * AngleStep;
		const FVector CirclePoint = Center + FVector(
			FMath::Cos(Angle) * Radius,
			FMath::Sin(Angle) * Radius,
			0.0f
		);

		// Trace from high above to far below
		const FVector TraceStart = FVector(CirclePoint.X, CirclePoint.Y, Center.Z + TraceUpDistance);
		const FVector TraceEnd = FVector(CirclePoint.X, CirclePoint.Y, Center.Z - TraceDownDistance);

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			// Ground found - offset upward by GroundOffset
			const FVector GroundPoint = Hit.ImpactPoint + FVector(0.0f, 0.0f, GroundOffset);
			GroundPoints.Add(GroundPoint);
		}
		else
		{
			// No ground found - use Center's Z height + GroundOffset as fallback
			const FVector FallbackPoint = FVector(CirclePoint.X, CirclePoint.Y, Center.Z + GroundOffset);
			GroundPoints.Add(FallbackPoint);
		}
	}

	return GroundPoints;
}

void ACombatRangeVisualizer::GenerateWallSegments(FVector Center, float Radius)
{
	if (bWallGenerated) return;

	if (!WallSegmentMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatRangeVisualizer] WallSegmentMesh is not assigned!"));
		return;
	}

	TArray<FVector> GroundPoints = SampleGroundPoints(Center, Radius, SampleCount);
	if (GroundPoints.Num() < 3) return;

	WallSpline->ClearSplinePoints();

	for (int32 i = 0; i < GroundPoints.Num(); ++i)
	{
		WallSpline->AddSplinePoint(GroundPoints[i], ESplineCoordinateSpace::World, false);
	}
	WallSpline->AddSplinePoint(GroundPoints[0], ESplineCoordinateSpace::World, false);
	WallSpline->SetClosedLoop(true, false);
	WallSpline->UpdateSpline();

	const int32 NumSegments = GroundPoints.Num();
	WallSegments.Reserve(NumSegments);

	for (int32 i = 0; i < NumSegments; ++i)
	{
		FString ComponentName = FString::Printf(TEXT("WallSegment_%d"), i);
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(
			this,
			USplineMeshComponent::StaticClass(),
			FName(*ComponentName)
		);

		if (!SplineMesh) continue;

		// Set properties before attaching
		SplineMesh->SetMobility(EComponentMobility::Movable);
		SplineMesh->SetCastShadow(false);
		SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SplineMesh->SetStaticMesh(WallSegmentMesh);

		// Attach and register
		SplineMesh->SetupAttachment(WallSpline);
		SplineMesh->RegisterComponent();

		// Set material after registration
		if (WallMaterial)
		{
			SplineMesh->SetMaterial(0, WallMaterial);
		}

		SplineMesh->SetVisibility(true);

		// Set spline mesh positions and scale
		const FVector StartPos = WallSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
		const FVector StartTangent = WallSpline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local);
		const FVector EndPos = WallSpline->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);
		const FVector EndTangent = WallSpline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);

		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent, true);

		// Set scale: X = Thickness (depth), Y = Height (vertical)
		const float MeshBaseSize = 100.0f;
		const FVector2D Scale(WallThickness, WallHeight / MeshBaseSize);
		SplineMesh->SetStartScale(Scale, true);
		SplineMesh->SetEndScale(Scale, true);

		WallSegments.Add(SplineMesh);
	}

	bWallGenerated = true;
}