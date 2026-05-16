// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/DelayedRuneProjectile.h"

#include "Combat/CombatTargetFilter.h"
#include "Characters/Core/BaseCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"

namespace CombatTargetFilter = MageBossDuel::CombatTargetFilter;

ADelayedRuneProjectile::ADelayedRuneProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Tags.Add(TEXT("BossSkillActor"));

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	RootComponent = CollisionComp;

	CollisionComp->InitSphereRadius(18.0f);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetGenerateOverlapEvents(true);
	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ADelayedRuneProjectile::OnProjectileBeginOverlap);
	CollisionComp->OnComponentHit.AddDynamic(this, &ADelayedRuneProjectile::OnProjectileHit);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CollisionComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 1300.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bAutoActivate = false;

	InitialLifeSpan = 8.0f;

	HitPayload.Damage = 12.0f;
	HitPayload.PoiseDamage = 25.0f;
	HitPayload.ReactionType = EHitReactionType::LightStagger;
	HitPayload.bCanInterrupt = true;
	HitPayload.bForceReaction = false;
	HitPayload.bIgnorePoise = false;
}

void ADelayedRuneProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* OwnerActor = GetOwner())
	{
		CollisionComp->IgnoreActorWhenMoving(OwnerActor, true);
	}

	if (AActor* InstigatorActor = GetInstigator())
	{
		CollisionComp->IgnoreActorWhenMoving(InstigatorActor, true);
	}
}

void ADelayedRuneProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActivationTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ADelayedRuneProjectile::InitializeRune(ABaseCharacter* InDamageCauser, AActor* InTarget, float InActivationDelay, float InAimYawOffsetDegrees)
{
	DamageCauser = InDamageCauser;
	TargetActor = InTarget;

	ActivationDelay = FMath::Max(0.0f, InActivationDelay);
	AimYawOffsetDegrees = InAimYawOffsetDegrees;

	SetLifeSpan(FMath::Max(ActivationDelay + LifeSpanAfterActivation + 1.0f, 1.0f));

	OnRuneInitialized(ActivationDelay);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ActivationDelay <= 0.0f)
	{
		ActivateRune();
		return;
	}

	World->GetTimerManager().SetTimer(
		ActivationTimerHandle,
		this,
		&ADelayedRuneProjectile::ActivateRune,
		ActivationDelay,
		false
	);
}

void ADelayedRuneProjectile::ActivateRune()
{
	if (bActivated || bHasImpacted)
	{
		return;
	}

	bActivated = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActivationTimerHandle);
	}

	const FVector AimDirection = GetAimDirection();

	if (AimDirection.IsNearlyZero())
	{
		Destroy();
		return;
	}

	SetActorRotation(AimDirection.Rotation());

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = ProjectileSpeed;
		ProjectileMovement->MaxSpeed = ProjectileSpeed;
		ProjectileMovement->Velocity = AimDirection * ProjectileSpeed;
		ProjectileMovement->Activate(true);
	}

	SetLifeSpan(LifeSpanAfterActivation);

	OnRuneActivated();
}

void ADelayedRuneProjectile::CancelRune()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActivationTimerHandle);
	}

	Destroy();
}

void ADelayedRuneProjectile::SetHitPayload(const FHitPayload& NewHitPayload)
{
	HitPayload = NewHitPayload;
	HitPayload.Damage = FMath::Max(0.0f, HitPayload.Damage);
	HitPayload.PoiseDamage = FMath::Max(0.0f, HitPayload.PoiseDamage);
}

void ADelayedRuneProjectile::OnProjectileBeginOverlap(
	UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult
)
{
	if (!bActivated || bHasImpacted)
	{
		return;
	}

	if (CombatTargetFilter::ShouldIgnoreActorForDamage(OtherActor, this, DamageCauser.Get()))
	{
		return;
	}

	const FVector ImpactLocation = SweepResult.bBlockingHit
		? FVector(SweepResult.ImpactPoint)
		: GetActorLocation();

	HandleImpact(OtherActor, ImpactLocation);
}

void ADelayedRuneProjectile::OnProjectileHit(
	UPrimitiveComponent* HitComponent, 
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, 
	FVector NormalImpulse,
	const FHitResult& Hit
)
{
	if (!bActivated || bHasImpacted)
	{
		return;
	}

	if (OtherActor && CombatTargetFilter::ShouldIgnoreActorForDamage(OtherActor, this, DamageCauser.Get()))
	{
		return;
	}

	const FVector ImpactLocation = Hit.bBlockingHit
		? FVector(Hit.ImpactPoint)
		: GetActorLocation();

	HandleImpact(OtherActor, ImpactLocation);
}

FVector ADelayedRuneProjectile::GetAimDirection() const
{
	const AActor* Target = TargetActor.Get();

	if (IsValid(Target))
	{
		const FVector TargetLocation =
			Target->GetActorLocation()
			+ FVector::UpVector * AimHeightOffset
			+ Target->GetVelocity() * TargetLeadSeconds;

		FVector AimDirection = (TargetLocation - GetActorLocation()).GetSafeNormal();

		if (!AimDirection.IsNearlyZero() && !FMath::IsNearlyZero(AimYawOffsetDegrees))
		{
			const FQuat YawOffsetQuat(
				FVector::UpVector,
				FMath::DegreesToRadians(AimYawOffsetDegrees)
			);

			AimDirection = YawOffsetQuat.RotateVector(AimDirection).GetSafeNormal();
		}
		return AimDirection;
	}
	return GetActorForwardVector();
}

void ADelayedRuneProjectile::HandleImpact(AActor* HitActor, const FVector& ImpactLocation)
{
	if (bHasImpacted)
	{
		return;
	}

	bHasImpacted = true;

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}

	SetActorEnableCollision(false);

	ABaseCharacter* Causer = DamageCauser.Get();

	if (ABaseCharacter* HitCharacter = CombatTargetFilter::GetAliveDamageTarget(HitActor))
	{
		if (!CombatTargetFilter::ShouldIgnoreActorForDamage(HitCharacter, this, Causer))
		{
			HitCharacter->ApplyHitPayload(HitPayload, Causer);
		}
	}

	OnRuneImpacted(ImpactLocation, HitActor);

	Destroy();
}
