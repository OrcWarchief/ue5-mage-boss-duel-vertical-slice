// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/RunePrisonBeamSegment.h"

#include "Characters/Core/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ARunePrisonBeamSegment::ARunePrisonBeamSegment()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComp"));
	RootComponent = CollisionComp;

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetGenerateOverlapEvents(true);

	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ARunePrisonBeamSegment::OnSegmentBeginOverlap);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CollisionComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARunePrisonBeamSegment::InitializeSegment(ABaseCharacter* InDamageCauser, const FVector& StartLocation, const FVector& EndLocation, float InBeamThickness, float InBeamHeight, const FHitPayload& InHitPayload)
{
	DamageCauser = InDamageCauser;
	HitPayload = InHitPayload;

	HitPayload.Damage = FMath::Max(0.0f, HitPayload.Damage);
	HitPayload.PoiseDamage = FMath::Max(0.0f, HitPayload.PoiseDamage);

	const FVector Delta = EndLocation - StartLocation;
	const float Length = Delta.Size();

	if (Length <= KINDA_SMALL_NUMBER)
	{
		Destroy();
		return;
	}

	const FVector Midpoint = (StartLocation + EndLocation) * 0.5f;
	const FRotator SegmentRotation = Delta.Rotation();

	SetActorLocationAndRotation(Midpoint, SegmentRotation);

	const FVector BoxExtent(
		Length * 0.5f,
		FMath::Max(1.0f, InBeamThickness * 0.5f),
		FMath::Max(1.0f, InBeamHeight * 0.5f)
	);

	CollisionComp->SetBoxExtent(BoxExtent, true);

	if (AActor* CauserActor = DamageCauser.Get())
	{
		CollisionComp->IgnoreActorWhenMoving(CauserActor, true);
	}

	bSegmentActive = false;
	DamagedActors.Reset();

	OnSegmentInitialized(StartLocation, EndLocation, InBeamThickness, InBeamHeight);

	if (bDrawDebugSegment)
	{
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			EndLocation,
			FColor::Purple,
			false,
			2.0f,
			0,
			InBeamThickness
		);
	}
}

void ARunePrisonBeamSegment::ActivateSegment()
{
	if (bSegmentActive)
	{
		return;
	}

	bSegmentActive = true;

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->UpdateOverlaps();

	OnSegmentActivated();

	TArray<AActor*> OverlappingActors;
	CollisionComp->GetOverlappingActors(OverlappingActors, ABaseCharacter::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		ApplyDamageToActor(Actor);
	}
}

void ARunePrisonBeamSegment::OnSegmentBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!bSegmentActive)
	{
		return;
	}

	ApplyDamageToActor(OtherActor);
}

bool ARunePrisonBeamSegment::IsIgnoredActor(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return true;
	}

	if (Actor == this)
	{
		return true;
	}

	if (Actor == GetOwner())
	{
		return true;
	}

	if (Actor == GetInstigator())
	{
		return true;
	}

	if (Actor == DamageCauser.Get())
	{
		return true;
	}

	return false;
}

void ARunePrisonBeamSegment::ApplyDamageToActor(AActor* Actor)
{
	if (IsIgnoredActor(Actor))
	{
		return;
	}

	if (DamagedActors.Contains(Actor))
	{
		return;
	}

	ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(Actor);
	if (!HitCharacter || !HitCharacter->IsAlive())
	{
		return;
	}

	ABaseCharacter* Causer = DamageCauser.Get();

	if (HitCharacter == Causer)
	{
		return;
	}

	HitCharacter->ApplyHitPayload(HitPayload, Causer);
	DamagedActors.Add(Actor);

	OnSegmentHitActor(Actor);
}