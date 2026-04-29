// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/BaseMagicProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Characters/Core/BaseCharacter.h"

// Sets default values
ABaseMagicProjectile::ABaseMagicProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	RootComponent = CollisionComp;
	CollisionComp->InitSphereRadius(16.f);

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetGenerateOverlapEvents(true);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABaseMagicProjectile::OnProjectileBeginOverlap);
	CollisionComp->OnComponentHit.AddDynamic(this, &ABaseMagicProjectile::OnProjectileHit);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CollisionComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 1600.f;
	ProjectileMovement->MaxSpeed = 1600.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 3.f;

	HitPayload.Damage = 10.0f;
	HitPayload.PoiseDamage = 10.0f;
	HitPayload.ReactionType = EHitReactionType::LightStagger;
	HitPayload.bCanInterrupt = true;
	HitPayload.bForceReaction = false;
	HitPayload.bIgnorePoise = false;
}

// Called when the game starts or when spawned
void ABaseMagicProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		CollisionComp->IgnoreActorWhenMoving(OwnerActor, true);
	}

	AActor* InstigatorActor = GetInstigator();
	if (InstigatorActor && InstigatorActor != OwnerActor)
	{
		CollisionComp->IgnoreActorWhenMoving(InstigatorActor, true);
	}

	ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
}

void ABaseMagicProjectile::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor) || !IsValid(OtherComp))
	{
		return;
	}

	if (OtherActor == this || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}
	
	ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(OtherActor);
	if (!HitCharacter || !HitCharacter->IsAlive())
	{
		return;
	}

	ABaseCharacter* AttackCharacter = Cast<ABaseCharacter>(GetOwner());
	if (!AttackCharacter)
	{
		AttackCharacter = Cast<ABaseCharacter>(GetInstigator());
	}

	if (HitCharacter == AttackCharacter)
	{
		return;
	}

	HitCharacter->ApplyHitPayload(HitPayload, AttackCharacter);

	Destroy();
}

void ABaseMagicProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!IsValid(OtherActor) || OtherActor == this || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	Destroy();
}

// Called every frame
void ABaseMagicProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseMagicProjectile::SetDamage(float NewDamage)
{
	HitPayload.Damage = FMath::Max(0.0f, NewDamage);
}

void ABaseMagicProjectile::SetHitPayload(const FHitPayload& NewHitPayload)
{
	HitPayload = NewHitPayload;
	HitPayload.Damage = FMath::Max(0.0f, HitPayload.Damage);
	HitPayload.PoiseDamage = FMath::Max(0.0f, HitPayload.PoiseDamage);
}