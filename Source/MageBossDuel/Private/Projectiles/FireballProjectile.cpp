// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/FireballProjectile.h"

#include "Combat/CombatTargetFilter.h"
#include "Characters/Core/BaseCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/ProjectileMovementComponent.h"

namespace CombatTargetFilter = MageBossDuel::CombatTargetFilter;

AFireballProjectile::AFireballProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Tags.Add(TEXT("BossSkillActor"));

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	RootComponent = CollisionComp;

	CollisionComp->InitSphereRadius(24.f);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetGenerateOverlapEvents(true);
	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AFireballProjectile::OnProjectileBeginOverlap);
	CollisionComp->OnComponentHit.AddDynamic(this, &AFireballProjectile::OnProjectileHit);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CollisionComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 1300.0f;
	ProjectileMovement->MaxSpeed = 1300.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	InitialLifeSpan = 4.0f;

	DirectHitPayload.Damage = 30.0f;
	DirectHitPayload.PoiseDamage = 100.0f;
	DirectHitPayload.ReactionType = EHitReactionType::Knockdown;
	DirectHitPayload.bCanInterrupt = true;
	DirectHitPayload.bForceReaction = false;
	DirectHitPayload.bIgnorePoise = false;

	SplashHitPayload.Damage = 15.0f;
	SplashHitPayload.PoiseDamage = 40.0f;
	SplashHitPayload.ReactionType = EHitReactionType::HeavyStagger;
	SplashHitPayload.bCanInterrupt = true;
	SplashHitPayload.bForceReaction = false;
	SplashHitPayload.bIgnorePoise = false;
}

void AFireballProjectile::BeginPlay()
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

	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
	}
}

void AFireballProjectile::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHasExploded)
	{
		return;
	}

	if (CombatTargetFilter::ShouldIgnoreActorForDamage(OtherActor, this))
	{
		return;
	}

	Explode(OtherActor);
}

void AFireballProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)
{
	if (bHasExploded)
	{
		return;
	}

	if (CombatTargetFilter::ShouldIgnoreActorForDamage(OtherActor, this))
	{
		return;
	}

	Explode(OtherActor);
}

void AFireballProjectile::Explode(AActor* DirectHitActor)
{
	if (bHasExploded)
	{
		return;
	}

	bHasExploded = true;

	const FVector ExplosionOrigin = GetActorLocation();

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}

	SetActorEnableCollision(false);

	if (MeshComp)
	{
		MeshComp->SetVisibility(false, true);
	}

	ApplyExplosionDamage(DirectHitActor, ExplosionOrigin);
	
	if (bDrawDebugExplosion)
	{
		DrawDebugSphere(
			GetWorld(),
			ExplosionOrigin,
			ExplosionRadius,
			24,
			FColor::Orange,
			false,
			1.0f
		);
	}

	OnFireballExploded(ExplosionOrigin, DirectHitActor);

	Destroy();
}

void AFireballProjectile::ApplyExplosionDamage(AActor* DirectHitActor, const FVector& ExplosionOrigin)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ABaseCharacter* DamageCauser = CombatTargetFilter::ResolveDamageCauser(this);

	TSet<AActor*> DamagedActors;

	// ===== Direct Hit =====

	if (!CombatTargetFilter::ShouldIgnoreActorForDamage(DirectHitActor, this, DamageCauser))
	{
		if (ABaseCharacter* DirectHitCharacter = Cast<ABaseCharacter>(DirectHitActor))
		{
			if (DirectHitCharacter->IsAlive() && DirectHitCharacter != DamageCauser)
			{
				DirectHitCharacter->ApplyHitPayload(DirectHitPayload, DamageCauser);
				DamagedActors.Add(DirectHitCharacter);
			}
		}
	}

	// ===== Splash Hit =====

	TArray<FOverlapResult> OverlapResults;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(FName(TEXT("FireballExplosion")), false);

	CombatTargetFilter::AddIgnoredActorsForDamageQuery(QueryParams, this, DamageCauser);

	const FCollisionShape ExplosionShape = FCollisionShape::MakeSphere(ExplosionRadius);

	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		ExplosionOrigin,
		FQuat::Identity,
		ObjectQueryParams,
		ExplosionShape,
		QueryParams
	);

	if (!bHasOverlap)
	{
		return;
	}

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* OverlappedActor = Result.GetActor();
		
		if (CombatTargetFilter::ShouldIgnoreActorForDamage(OverlappedActor, this, DamageCauser))
		{
			continue;
		}

		if (DamagedActors.Contains(OverlappedActor))
		{
			continue;
		}

		ABaseCharacter* HitCharacter = CombatTargetFilter::GetAliveDamageTarget(OverlappedActor);
		if (!HitCharacter)
		{
			continue;
		}

		HitCharacter->ApplyHitPayload(SplashHitPayload, DamageCauser);
		DamagedActors.Add(HitCharacter);
	}
}