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

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetGenerateOverlapEvents(true);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABaseMagicProjectile::OnProjectileBeginOverlap);

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

	HitCharacter->ApplyDamage(Damage, AttackCharacter);

	Destroy();
}

// Called every frame
void ABaseMagicProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseMagicProjectile::SetDamage(float NewDamage)
{
	Damage = FMath::Max(0.f, NewDamage);
}

