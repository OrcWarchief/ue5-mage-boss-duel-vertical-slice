// Fill out your copyright notice in the Description page of Project Settings.


#include "GamePlay/BossRoomEntranceTrigger.h"

#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GamePlay/DuelEncounterManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/HUD/DuelScreenFadeWidget.h"

ABossRoomEntranceTrigger::ABossRoomEntranceTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(SceneRoot);

	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ABossRoomEntranceTrigger::BeginPlay()
{
	Super::BeginPlay();
	
	if (TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(
			this, 
			&ABossRoomEntranceTrigger::HandleTriggerBeginOverlap
		);
	}
}

void ABossRoomEntranceTrigger::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult
)
{
	if (bEntryInProgress)
	{
		return;
	}

	if (bTriggerOnlyOnce && bHasTriggered)
	{
		return;
	}

	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (!IsValid(PlayerPawn) || !PlayerPawn->IsPlayerControlled() )
	{
		return;
	}

	BeginBossRoomEntry(PlayerPawn);
}

void ABossRoomEntranceTrigger::BeginBossRoomEntry(APawn* PlayerPawn)
{
	if (!IsValid(PlayerPawn))
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(PlayerPawn->GetController());
	if (!IsValid(PlayerController))
	{
		return;
	}

	if (!IsValid(EncounterManager))
	{
		UE_LOG(LogTemp, Warning, TEXT("BossRoomEntranceTrigger: EncounterManager reference is not set."));
		return;
	}

	if (!IsValid(ArenaEntryPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("BossRoomEntranceTrigger: ArenaEntryPoint reference is not set."));
		return;
	}

	bEntryInProgress = true;
	bHasTriggered = true;

	PendingPawn = PlayerPawn;
	PendingPlayerController = PlayerController;

	SetPlayerTransitionInputLocked(PlayerController, true);

	if (ScreenFadeWidgetClass)
	{
		ActiveFadeWidget = CreateWidget<UDuelScreenFadeWidget>(PlayerController, ScreenFadeWidgetClass);
		if (ActiveFadeWidget)
		{
			ActiveFadeWidget->AddToViewport(ScreenFadeZOrder);
			ActiveFadeWidget->RequestFadeIn();
		}
	}

	OnBossRoomEntryStarted(PlayerPawn);

	UWorld* World = GetWorld();
	if (!World)
	{
		TeleportPlayerToArenaEntry();
		return;
	}

	World->GetTimerManager().SetTimer(
		TeleportTimerHandle,
		this,
		&ABossRoomEntranceTrigger::TeleportPlayerToArenaEntry,
		FadeInDuration + BlackHoldDuration,
		false
	);
}

void ABossRoomEntranceTrigger::TeleportPlayerToArenaEntry()
{
	APawn* PlayerPawn = PendingPawn.Get();
	APlayerController* PlayerController = PendingPlayerController.Get();

	if (!IsValid(PlayerPawn) || !IsValid(PlayerController))
	{
		FinishBossRoomEntry();
		return;
	}

	if (UPawnMovementComponent* MovementComp = PlayerPawn->GetMovementComponent())
	{
		MovementComp->StopMovementImmediately();
	}

	FVector EntryLocation = ArenaEntryPoint->GetActorLocation();
	FRotator EntryRotation = ArenaEntryPoint->GetActorRotation();
	EntryRotation.Pitch = 0.0f; // Ensure the player doesn't get teleported with an unintended pitch
	EntryRotation.Roll = 0.0f;  // Ensure the player doesn't get teleported with an unintended roll

	PlayerPawn->TeleportTo(
		EntryLocation, 
		EntryRotation,
		false,
		true
	);
}

void ABossRoomEntranceTrigger::BeginFadeOut()
{
	if (ActiveFadeWidget)
	{
		ActiveFadeWidget->RequestFadeOut();
	}
	UWorld* World = GetWorld();
	if (!World || FadeOutDuration <= 0.0f)
	{
		FinishBossRoomEntry();
		return;
	}

	World->GetTimerManager().SetTimer(
		FadeOutTimerHandle,
		this,
		&ABossRoomEntranceTrigger::FinishBossRoomEntry,
		FadeOutDuration,
		false
	);
}

void ABossRoomEntranceTrigger::FinishBossRoomEntry()
{
	APawn* PlayerPawn = PendingPawn.Get();
	APlayerController* PlayerController = PendingPlayerController.Get();

	if (IsValid(ActiveFadeWidget))
	{
		ActiveFadeWidget->RemoveFromParent();
		ActiveFadeWidget = nullptr;
	}

	if (IsValid(EncounterManager))
	{
		EncounterManager->StartEncounter();
	}

	if (IsValid(PlayerController))
	{
		SetPlayerTransitionInputLocked(PlayerController, false);
	}

	if (IsValid(PlayerPawn))
	{
		OnBossRoomEntryFinished(PlayerPawn);
	}

	PendingPawn = nullptr;
	PendingPlayerController = nullptr;
	bEntryInProgress = false;
}

void ABossRoomEntranceTrigger::SetPlayerTransitionInputLocked(APlayerController* PlayerController, bool bLocked) const
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->SetIgnoreMoveInput(bLocked);
	PlayerController->SetIgnoreLookInput(bLocked);
}
