// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/RestPointActor.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/MBDRespawnSubsystem.h"

ARestPointActor::ARestPointActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	RespawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(SceneRoot);
}

void ARestPointActor::ActivateRestPoint(APawn* ActivatingPawn)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UMBDRespawnSubsystem* RespawnSubsystem = GameInstance->GetSubsystem<UMBDRespawnSubsystem>();

	if (!RespawnSubsystem)
	{
		return;
	}

	const FName CurrentLevelName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));

	RespawnSubsystem->SetActiveRestPoint(
		ResolveRestPointId(),
		CurrentLevelName,
		GetRespawnTransform()
	);

	OnRestPointActivated(ActivatingPawn);
}

FTransform ARestPointActor::GetRespawnTransform() const
{
	return RespawnPoint ? RespawnPoint->GetComponentTransform() : GetActorTransform();
}

void ARestPointActor::BeginPlay()
{
	Super::BeginPlay();

	if (bDrawDebugRespawnPoint)
	{
		DrawDebugCoordinateSystem(
			GetWorld(),
			GetRespawnTransform().GetLocation(),
			GetRespawnTransform().Rotator(),
			80.0f,
			false,
			5.0f
		);
	}

	if (bSetAsDefaultOnBeginPlay)
	{
		ActivateRestPoint(nullptr);
	}
}

FName ARestPointActor::ResolveRestPointId() const
{
	if (!RestPointId.IsNone())
	{
		return RestPointId;
	}

	return GetFName();
}

