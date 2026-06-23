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
#include "Characters/Player/PlayerCharacter.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameInstance.h"

ARestPointActor::ARestPointActor()
{
	PrimaryActorTick.bCanEverTick = false;

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

	InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
	InteractionPromptWidget->SetupAttachment(SceneRoot);
	InteractionPromptWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 140.0f));
	InteractionPromptWidget->SetWidgetSpace(EWidgetSpace::Screen);
	InteractionPromptWidget->SetDrawAtDesiredSize(true);
	InteractionPromptWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionPromptWidget->SetGenerateOverlapEvents(false);
	InteractionPromptWidget->SetHiddenInGame(true);
}

void ARestPointActor::ActivateRestPoint(APawn* ActivatingPawn)
{
	CommitRestPointActivation(ActivatingPawn);
}

bool ARestPointActor::TryActivateRestPoint(APawn* ActivatingPawn)
{
	if (!CanActivateRestPoint(ActivatingPawn))
	{
		OnRestPointActivationFailed(ActivatingPawn);
		return false;
	}

	if (!CommitRestPointActivation(ActivatingPawn))
	{
		OnRestPointActivationFailed(ActivatingPawn);
		return false;
	}

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

	SetInteractionPromptVisible(false);

	if (InteractionVolume)
	{
		InteractionVolume->OnComponentBeginOverlap.AddDynamic(
			this,
			&ARestPointActor::HandleInteractionBeginOverlap
		);

		InteractionVolume->OnComponentEndOverlap.AddDynamic(
			this,
			&ARestPointActor::HandleInteractionEndOverlap
		);
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
		RegisterAsActiveRestPoint();
	}
}

void ARestPointActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(FocusedPawn.Get()))
	{
		PlayerCharacter->ClearFocusedRestPoint(this);
	}

	FocusedPawn.Reset();
	SetInteractionPromptVisible(false);

	Super::EndPlay(EndPlayReason);
}

FName ARestPointActor::ResolveRestPointId() const
{
	if (!RestPointId.IsNone())
	{
		return RestPointId;
	}

	return GetFName();
}

bool ARestPointActor::RegisterAsActiveRestPoint()
{
	UGameInstance* GameInstance = GetGameInstance();

	if (!GameInstance)
	{
		return false;
	}

	UMBDRespawnSubsystem* RespawnSubsystem = GameInstance->GetSubsystem<UMBDRespawnSubsystem>();

	if (!RespawnSubsystem)
	{
		return false;
	}

	const FName CurrentLevelName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));

	RespawnSubsystem->SetActiveRestPoint(
		ResolveRestPointId(),
		CurrentLevelName,
		GetRespawnTransform()
	);

	return true;
}

bool ARestPointActor::CommitRestPointActivation(APawn* ActivatingPawn)
{
	if (!RegisterAsActiveRestPoint())
	{
		return false;
	}

	if (!IsValid(ActivatingPawn))
	{
		return false;
	}

	if (!ActivatingPawn->IsPlayerControlled())
	{
		return false;
	}

	OnRestPointActivated(ActivatingPawn);

	return true;
}

APlayerCharacter* ARestPointActor::ResolvePlayerCharacter(AActor* OtherActor) const
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);

	if (!IsValid(PlayerCharacter))
	{
		return nullptr;
	}

	if (!PlayerCharacter->IsPlayerControlled())
	{
		return nullptr;
	}

	return PlayerCharacter;
}

void ARestPointActor::SetInteractionPromptVisible(bool bVisible)
{
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetHiddenInGame(!bVisible);
	}
}

void ARestPointActor::HandleInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerCharacter* PlayerCharacter = ResolvePlayerCharacter(OtherActor);

	if (!PlayerCharacter)
	{
		return;
	}

	if (FocusedPawn.Get() == PlayerCharacter)
	{
		return;
	}

	if (FocusedPawn.IsValid())
	{
		return;
	}

	FocusedPawn = PlayerCharacter;

	PlayerCharacter->SetFocusedRestPoint(this);

	SetInteractionPromptVisible(true);

	OnRestPointFocusChanged(PlayerCharacter, true);
}

void ARestPointActor::HandleInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor);

	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	if (FocusedPawn.Get() != PlayerCharacter)
	{
		return;
	}

	if (InteractionVolume && InteractionVolume->IsOverlappingActor(PlayerCharacter))
	{
		return;
	}

	PlayerCharacter->ClearFocusedRestPoint(this);

	FocusedPawn.Reset();

	SetInteractionPromptVisible(false);

	OnRestPointFocusChanged(PlayerCharacter, false);
}

