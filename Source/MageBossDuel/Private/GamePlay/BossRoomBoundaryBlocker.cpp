// Fill out your copyright notice in the Description page of Project Settings.


#include "GamePlay/BossRoomBoundaryBlocker.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ABossRoomBoundaryBlocker::ABossRoomBoundaryBlocker()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockingVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingVolume"));
	BlockingVolume->SetupAttachment(SceneRoot);

	BlockingVolume->SetBoxExtent(FVector(220.0f, 40.0f, 220.0f));
	BlockingVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BlockingVolume->SetCollisionObjectType(ECC_WorldStatic);
	BlockingVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BlockingVolume->SetGenerateOverlapEvents(false);
	BlockingVolume->SetHiddenInGame(true);
}

void ABossRoomBoundaryBlocker::BeginPlay()
{
	Super::BeginPlay();

	SetBlocked(bBlockedOnBeginPlay);
}

void ABossRoomBoundaryBlocker::SetBlocked(bool bBlocked)
{
	bIsBlocked = bBlocked;

	if (BlockingVolume)
	{
		BlockingVolume->SetCollisionEnabled(
			bIsBlocked
			? ECollisionEnabled::QueryAndPhysics
			: ECollisionEnabled::NoCollision
		);
	}

	if (bDrawDebugBlocker && BlockingVolume)
	{
		DrawDebugBox(
			GetWorld(),
			BlockingVolume->GetComponentLocation(),
			BlockingVolume->GetScaledBoxExtent(),
			BlockingVolume->GetComponentQuat(),
			bIsBlocked ? FColor::Red : FColor::Green,
			false,
			2.0f
		);
	}

	OnBoundaryBlockedChanged(bIsBlocked);
}
