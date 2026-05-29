// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/RestPointActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
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

	InteractionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(SceneRoot);
	InteractionVolume->InitSphereRadius(180.0f);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionVolume->SetGenerateOverlapEvents(true);
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

bool ARestPointActor::TryActivateRestPoint(APawn* ActivatingPawn)
{
	if (!CanActivateRestPoint(ActivatingPawn))
	{
		OnRestPointActivationFailed(ActivatingPawn);
		return false;
	}

	ActivateRestPoint(ActivatingPawn);
	return true;
}

bool ARestPointActor::CanActivateRestPoint(APawn* ActivatingPawn) const
{
	if (!IsValid(ActivatingPawn))
	{
		return false;
	}

	if (!ActivatingPawn->IsPlayerControlled())
	{
		return false;
	}

	if (bRequirePlayerOverlapForActivation && FocusedPawn.Get() != ActivatingPawn)
	{
		return false;
	}

	return true;
}

FTransform ARestPointActor::GetRespawnTransform() const
{
	return RespawnPoint ? RespawnPoint->GetComponentTransform() : GetActorTransform();
}

void ARestPointActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionVolume)
	{
		InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &ARestPointActor::HandleInteractionBeginOverlap);
		InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &ARestPointActor::HandleInteractionEndOverlap);
	}

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

void ARestPointActor::HandleInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!IsValid(Pawn))
	{
		return;
	}

	FocusedPawn = Pawn;

	OnRestPointFocusChanged(Pawn, true);
}

void ARestPointActor::HandleInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!IsValid(Pawn))
	{
		return;
	}

	if (FocusedPawn.Get() != Pawn)
	{
		return;
	}

	FocusedPawn = nullptr;

	OnRestPointFocusChanged(Pawn, false);
}

