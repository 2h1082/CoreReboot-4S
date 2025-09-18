#include "MonsterAI/Skills/ColdFairyActor.h"
#include "CR4S.h"
#include "NiagaraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "FriendlyAI/Component/ObjectPoolComponent.h"
#include "Game/System/ProjectilePoolSubsystem.h"

AColdFairyActor::AColdFairyActor()
{
	PrimaryActorTick.bCanEverTick = true;
	
	PoolComponent = CreateDefaultSubobject<UObjectPoolComponent>(TEXT("PoolComponent"));
	
	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
	CollisionComp->SetupAttachment(RootComp);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionProfileName(TEXT("MonsterSkillActor"));
	CollisionComp->SetGenerateOverlapEvents(true);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AColdFairyActor::OnOverlap);

	NiagaraComp->SetupAttachment(RootComp);
	
	StaticMesh->SetupAttachment(RootComp);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
	ProjectileMovementComp->UpdatedComponent = RootComp;
	ProjectileMovementComp->InitialSpeed = Speed;
	ProjectileMovementComp->MaxSpeed = MaxSpeed;
	ProjectileMovementComp->ProjectileGravityScale = 1.0f;
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	ProjectileMovementComp->bAutoActivate = false;
	ProjectileMovementComp->bIsHomingProjectile = false;
	ProjectileMovementComp->HomingAccelerationMagnitude = 0.f;
	ProjectileMovementComp->OnProjectileStop.AddDynamic(this, &AColdFairyActor::OnProjectileStop);
}

void AColdFairyActor::BeginPlay()
{
	Super::BeginPlay();

	CR4S_Log(LogDa, Warning, TEXT("[%s] BeginPlay - Damage : %f"), *GetClass()->GetName(), Damage);
	
	if (PoolComponent)
	{
		PoolComponent->OnReturnToPoolDelegate.AddDynamic(this, &AColdFairyActor::ResetProjectile);
	}

	if (IsValid(CollisionComp) && CollisionComp->IsRegistered())
	{
		if (APawn* InstPawn = GetInstigator<APawn>())
			CollisionComp->IgnoreActorWhenMoving(InstPawn, true);
	}

	if (IsValid(LaunchSkillSound))
	{
		PlaySkillSound(LaunchSkillSound);
	}
}

void AColdFairyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bHasLaunched && IsValid(TargetActor))
	{
		FVector ToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(ToTarget.Rotation());
		}
	}
}

void AColdFairyActor::OnProjectileStop(const FHitResult& ImpactResult)
{
	StopAndStick(ImpactResult);
}

void AColdFairyActor::InitialLaunch(AActor* InTarget, int32 InIndex, int32 InTotalCount)
{
	if (!InTarget) return;
	
	TargetActor = InTarget;
	SpawnOrder = InIndex;
	TotalCount = InTotalCount;

	bSequentialLaunch ? HandleSequenceLaunch() : HandleImmediateLaunch();
}

void AColdFairyActor::HandleSequenceLaunch()
{
	LaunchDelay = SpawnOrder * Interval;

	if (LaunchDelay <= KINDA_SMALL_NUMBER)
	{
		Launch();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			LaunchTimerHandle,
			this,
			&AColdFairyActor::Launch,
			LaunchDelay,
			false
		);
	}
}

void AColdFairyActor::HandleImmediateLaunch() const
{
	if (bHasLaunched) return;
	
	if (LaunchDelay <= KINDA_SMALL_NUMBER)
	{
		const_cast<AColdFairyActor*>(this)->Launch();
	}
	else
	{
		FTimerDelegate LaunchDelegate;
		LaunchDelegate.BindUObject(const_cast<AColdFairyActor*>(this), &AColdFairyActor::Launch);
		
		GetWorld()->GetTimerManager().SetTimer(
			const_cast<AColdFairyActor*>(this)->LaunchTimerHandle,
			LaunchDelegate,
			LaunchDelay,
			false
		);
	}
}

void AColdFairyActor::StopAndStick(const FHitResult& HitResult, AActor* HitActor, UPrimitiveComponent* HitComp)
{
	if (!ProjectileMovementComp->IsActive()) return;
	
	ProjectileMovementComp->StopMovementImmediately();
	ProjectileMovementComp->Deactivate();
	ProjectileMovementComp->bIsHomingProjectile = false;
	ProjectileMovementComp->Velocity = FVector::ZeroVector;
	
	if (StaticMesh)
	{
		StaticMesh->SetVisibility(false);
	}

	if (NiagaraComp)
	{
		NiagaraComp->SetVisibility(false);
	}
	
	if (IsValid(HitComp) && HitComp != CollisionComp)
	{
		AttachToComponent(HitComp, FAttachmentTransformRules::KeepWorldTransform);
	}
	else if (IsValid(HitActor) && HitComp != CollisionComp)
	{
		AttachToActor(HitActor, FAttachmentTransformRules::KeepWorldTransform);
	}
	
	if (IsValid(CollisionComp) && CollisionComp->IsRegistered())
	{
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComp->SetGenerateOverlapEvents(false);
	}
	
	FTimerDelegate ReturnDelegate;
	ReturnDelegate.BindLambda([this]() 
	{ 
		if (UWorld* World = GetWorld())
		{
			if (UProjectilePoolSubsystem* Pool = World->GetSubsystem<UProjectilePoolSubsystem>())
			{
				Pool->ReturnToPool(this);
			}
		}
	});
	
	GetWorld()->GetTimerManager().SetTimer(DestroyTimerHandle, ReturnDelegate, LifeTime, false);
}

void AColdFairyActor::Launch()
{
	if (!CR4S_VALIDATE(LogDa, IsValid(this))) return;
	if (!CR4S_VALIDATE(LogDa, IsValid(TargetActor))) return;
	if (bHasLaunched) return;

	bHasLaunched = true;
	SetActorTickEnabled(false);
	
	if (!ProjectileMovementComp->IsActive())
		ProjectileMovementComp->Activate(true);

	FVector Direction;
	if (IsValid(TargetActor))
	{
		Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	}
	else
	{
		Direction = GetActorForwardVector().GetSafeNormal();
	}
	
	SetActorRotation(Direction.Rotation());

	ProjectileMovementComp->InitialSpeed = Speed;
	ProjectileMovementComp->MaxSpeed = MaxSpeed;
	
	if (bUseHoming)
	{
		ProjectileMovementComp->ProjectileGravityScale = GravityScale;
		ProjectileMovementComp->bIsHomingProjectile = bUseHoming;
		ProjectileMovementComp->HomingAccelerationMagnitude = HomingAcceleration;
		ProjectileMovementComp->HomingTargetComponent = TargetActor->GetRootComponent();
		ProjectileMovementComp->bRotationFollowsVelocity = true;
	}
	else
	{
		ProjectileMovementComp->ProjectileGravityScale = 0.1f;
		ProjectileMovementComp->bIsHomingProjectile = false;
		ProjectileMovementComp->HomingAccelerationMagnitude = 2000.f;
		ProjectileMovementComp->HomingTargetComponent = nullptr;
		ProjectileMovementComp->bRotationFollowsVelocity = true; 
	}
	
	ProjectileMovementComp->Velocity = Direction * Speed;
	ProjectileMovementComp->UpdateComponentVelocity();
}

void AColdFairyActor::ResetProjectile()
{
	bHasLaunched = false;
	SpawnOrder = 0;
	TotalCount = 0;
	TargetActor = nullptr;
	LaunchDirection = FVector::ZeroVector;
	
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
		
	if (TimerManager.IsTimerActive(LaunchTimerHandle))
	{
		TimerManager.ClearTimer(LaunchTimerHandle);
	}
		
	if (TimerManager.IsTimerActive(DestroyTimerHandle))
	{
		TimerManager.ClearTimer(DestroyTimerHandle);
	}
	
	if (StaticMesh)
	{
		StaticMesh->SetVisibility(true);
	}

	if (NiagaraComp)
	{
		NiagaraComp->SetVisibility(true);
		NiagaraComp->Deactivate();
	}
	
	if (CollisionComp)
	{
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionComp->SetCollisionProfileName(TEXT("MonsterSkillActor"));
		CollisionComp->SetGenerateOverlapEvents(true);
	}
	
	if (ProjectileMovementComp)
	{
		ProjectileMovementComp->StopMovementImmediately();
		ProjectileMovementComp->Deactivate();
		ProjectileMovementComp->Velocity = FVector::ZeroVector;
		ProjectileMovementComp->bIsHomingProjectile = false;
		ProjectileMovementComp->HomingTargetComponent = nullptr;
	}
	
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
	SetInstigator(nullptr);
	
	if (AlreadyDamaged.Num() > 0)
	{
		AlreadyDamaged.Empty();
	}
}

void AColdFairyActor::OnOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherComp) return;
	if (OtherActor->IsA<AColdFairyActor>() || OtherActor->IsA(ABaseSkillActor::StaticClass())) return;
	if (OtherActor && OtherActor == GetInstigator()) return;
	
	Super::OnOverlap(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	
	StopAndStick(SweepResult, OtherActor, OtherComp);
}