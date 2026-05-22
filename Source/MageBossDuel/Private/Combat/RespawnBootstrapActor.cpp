// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/RespawnBootstrapActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/MBDRespawnSubsystem.h"
#include "TimerManager.h"

// Sets default values
ARespawnBootstrapActor::ARespawnBootstrapActor()
{
	PrimaryActorTick.bCanEverTick = false;

}

void ARespawnBootstrapActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ApplyRespawnTimerHandle,
			this,
			&ARespawnBootstrapActor::TryApplyPendingRespawn,
			RetryDelay,
			false
		);
	}
}

void ARespawnBootstrapActor::TryApplyPendingRespawn()
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

	if (!RespawnSubsystem->HasPendingRespawnAfterLevelLoad())
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn())
	{
		++ApplyAttempts;

		if (ApplyAttempts < MaxApplyAttempts)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					ApplyRespawnTimerHandle,
					this,
					&ARespawnBootstrapActor::TryApplyPendingRespawn,
					RetryDelay,
					false
				);
			}
		}

		return;
	}

	const bool bRespawned = RespawnSubsystem->RespawnPlayerAtActiveRestPoint(PC);
	if (bRespawned)
	{
		RespawnSubsystem->ConsumePendingRespawnAfterLevelLoad();
		return;
	}

	++ApplyAttempts;

	if (ApplyAttempts < MaxApplyAttempts)
	{
		++ApplyAttempts;
		if (ApplyAttempts < MaxApplyAttempts)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					ApplyRespawnTimerHandle,
					this,
					&ARespawnBootstrapActor::TryApplyPendingRespawn,
					RetryDelay,
					false
				);
			}
		}
	}
}
