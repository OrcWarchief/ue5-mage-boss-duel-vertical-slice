// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/RunePrisonBeamSegment.h"

#include "Combat/CombatTargetFilter.h"
#include "Characters/Core/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

namespace CombatTargetFilter = MageBossDuel::CombatTargetFilter;

ARunePrisonBeamSegment::ARunePrisonBeamSegment()
{
	PrimaryActorTick.bCanEverTick = false;

	Tags.Add(TEXT("BossSkillActor"));

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

void ARunePrisonBeamSegment::ApplyDamageToActor(AActor* Actor)
{
	ABaseCharacter* Causer = DamageCauser.Get();

	if (CombatTargetFilter::ShouldIgnoreActorForDamage(Actor, this, Causer))
	{
		return;
	}

	if (DamagedActors.Contains(Actor))
	{
		return;
	}

	ABaseCharacter* HitCharacter = CombatTargetFilter::GetAliveDamageTarget(Actor);
	if (!HitCharacter)
	{
		return;
	}

	HitCharacter->ApplyHitPayload(HitPayload, Causer);
	DamagedActors.Add(Actor);

	OnSegmentHitActor(Actor);
}