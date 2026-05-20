// Fill out your copyright notice in the Description page of Project Settings.


#include "GamePlay/DuelEncounterManager.h"

#include "Blueprint/UserWidget.h"
#include "Characters/Boss/MageBossCharacter.h"
#include "Characters/Core/BaseCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/HUD/BossEncounterHUDWidget.h"

ADuelEncounterManager::ADuelEncounterManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADuelEncounterManager::StartEncounter()
{
	if (bEncounterActive)
	{
		return;
	}
	bEncounterActive = true;
	bEndFlowStarted = false;
	CurrentEndResult = EDuelEndResult::None;

	if (ActiveEndWidget)
	{
		ActiveEndWidget->RemoveFromParent();
		ActiveEndWidget = nullptr;
	}

	ClearBossEncounterHUD();

	if (BossCharacter && PlayerCharacter)
	{
		BossCharacter->SetCombatTarget(PlayerCharacter);
		BossCharacter->StartBossBrain();
	}

	SetPlayerInputLocked(false, false);

	ShowBossEncounterHUD();

	OnEncounterStarted();
}

void ADuelEncounterManager::EndEncounter(EDuelEndResult Result)
{
	if (!bEncounterActive || bEndFlowStarted)
	{
		return;
	}
	bEncounterActive = false;
	bEndFlowStarted = true;
	CurrentEndResult = Result;

	if (BossCharacter)
	{
		BossCharacter->StopBossBrain();
	}

	if (bLockPlayerInputOnEnd)
	{
		const bool bLockLook =
			Result == EDuelEndResult::Victory
			? !bAllowCameraLookAfterVictory
			: true;

		SetPlayerInputLocked(true, bLockLook);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Delay =
		Result == EDuelEndResult::Victory
		? VictoryScreenDelay
		: DefeatScreenDelay;

	World->GetTimerManager().ClearTimer(EndScreenTimerHandle);

	if (Delay <= 0.0f)
	{
		ShowEndScreen();
	}
	else
	{
		World->GetTimerManager().SetTimer(
			EndScreenTimerHandle,
			this,
			&ADuelEncounterManager::ShowEndScreen,
			Delay,
			false
		);
	}

	if (Result == EDuelEndResult::Victory)
	{
		OnVictoryFlowStarted();
	}
	else if (Result == EDuelEndResult::Defeat)
	{
		OnDefeatFlowStarted();
	}
}

void ADuelEncounterManager::RestartEncounter()
{
	ClearBossEncounterHUD();

	if (ActiveEndWidget)
	{
		ActiveEndWidget->RemoveFromParent();
		ActiveEndWidget = nullptr;
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);

	const FName LevelToLoad = 
		RestartMapName.IsNone()
		? FName(*CurrentLevelName)
		: RestartMapName;

	UGameplayStatics::OpenLevel(this, LevelToLoad);
}

void ADuelEncounterManager::SetPlayerInputLocked(bool bLocked, bool bLockLook)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	PC->SetIgnoreMoveInput(bLocked);
	PC->SetIgnoreLookInput(bLocked && bLockLook);
}

void ADuelEncounterManager::BeginPlay()
{
	Super::BeginPlay();
	
	if (bAutoFindReferences)
	{
		AutoFindReferences();
	}

	BindDeathEvents();

	if (bAutoStartEncounter)
	{
		StartEncounter();
	}
}

void ADuelEncounterManager::AutoFindReferences()
{
	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<ABaseCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	}

	if (!BossCharacter)
	{
		for (TActorIterator<AMageBossCharacter> It(GetWorld()); It; ++It)
		{
			BossCharacter = *It;
			break;
		}
	}
}

void ADuelEncounterManager::BindDeathEvents()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->OnCharacterDeathStarted.AddDynamic(this, &ADuelEncounterManager::HandlePlayerDeathStarted);
		PlayerCharacter->OnCharacterDeathFinished.AddDynamic(this, &ADuelEncounterManager::HandlePlayerDeathFinished);
	}
	if (BossCharacter)
	{
		BossCharacter->OnCharacterDeathStarted.AddDynamic(this, &ADuelEncounterManager::HandleBossDeathStarted);
		BossCharacter->OnCharacterDeathFinished.AddDynamic(this, &ADuelEncounterManager::HandleBossDeathFinished);
	}
}

void ADuelEncounterManager::ShowEndScreen()
{
	HideBossEncounterHUD();

	if (ActiveEndWidget)
	{
		ActiveEndWidget->RemoveFromParent();
		ActiveEndWidget = nullptr;
	}

	TSubclassOf<UUserWidget> WidgetClass = nullptr;

	switch (CurrentEndResult)
	{
	case EDuelEndResult::Victory:
		WidgetClass = VictoryWidgetClass;
		break;

	case EDuelEndResult::Defeat:
		WidgetClass = DefeatWidgetClass;
		break;

	default:
		break;
	}

	if (WidgetClass)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			ActiveEndWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
			if (ActiveEndWidget)
			{
				ActiveEndWidget->AddToViewport(EndWidgetZOrder);
			}
		}
	}

	OnEndScreenReady(CurrentEndResult); // SFX / Camera / Shake 등 엔드 스크린과 동시에 시작되어야 하는 요소들을 블루프린트에서 처리할 수 있도록 이벤트 호출 
}

void ADuelEncounterManager::ShowBossEncounterHUD()
{
	if (!BossEncounterHUDWidgetClass)
	{
		return;
	}

	if (!BossCharacter)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	if (!ActiveBossEncounterHUDWidget)
	{
		ActiveBossEncounterHUDWidget =
			CreateWidget<UBossEncounterHUDWidget>(
				PC,
				BossEncounterHUDWidgetClass
			);

		if (ActiveBossEncounterHUDWidget)
		{
			ActiveBossEncounterHUDWidget->AddToViewport(BossEncounterHUDZOrder);
		}
	}

	if (ActiveBossEncounterHUDWidget)
	{
		ActiveBossEncounterHUDWidget->InitializeFromBoss(BossCharacter);
	}
}

void ADuelEncounterManager::HideBossEncounterHUD()
{
	if (!ActiveBossEncounterHUDWidget)
	{
		return;
	}

	ActiveBossEncounterHUDWidget->BeginHide();

	UWorld* World = GetWorld();
	if (!World || BossEncounterHUDHideRemoveDelay <= 0.0f)
	{
		RemoveBossEncounterHUD();
		return;
	}

	World->GetTimerManager().ClearTimer(BossEncounterHUDRemoveTimerHandle);
	World->GetTimerManager().SetTimer(
		BossEncounterHUDRemoveTimerHandle,
		this,
		&ADuelEncounterManager::RemoveBossEncounterHUD,
		BossEncounterHUDHideRemoveDelay,
		false
	);
}

void ADuelEncounterManager::RemoveBossEncounterHUD()
{
	if (!ActiveBossEncounterHUDWidget)
	{
		return;
	}

	ActiveBossEncounterHUDWidget->RemoveFromParent();
	ActiveBossEncounterHUDWidget = nullptr;
}

void ADuelEncounterManager::ClearBossEncounterHUD()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BossEncounterHUDRemoveTimerHandle);
	}

	if (!ActiveBossEncounterHUDWidget)
	{
		return;
	}

	ActiveBossEncounterHUDWidget->ClearBoss();
	ActiveBossEncounterHUDWidget->RemoveFromParent();
	ActiveBossEncounterHUDWidget = nullptr;
}

void ADuelEncounterManager::DestroyRemainingBossSkillActors()
{
	if (!GetWorld())
	{
		return;
	}

	TArray<AActor*> SkillActors;
	UGameplayStatics::GetAllActorsWithTag(
		this,
		BossSkillActorTag,
		SkillActors
	);

	for (AActor* Actor : SkillActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
}

void ADuelEncounterManager::HandleBossDeathStarted(ABaseCharacter* DeadCharacter)
{
	if (!bEncounterActive)
	{
		return;
	}

	if (BossCharacter)
	{
		BossCharacter->StopBossBrain();
	}

	if (bDestroyRemainingBossSkillActorsOnBossDeath)
	{
		DestroyRemainingBossSkillActors();
	}

	OnBossDeathStarted();
}

void ADuelEncounterManager::HandleBossDeathFinished(ABaseCharacter* DeadCharacter)
{
	// 보스 사망 시점에 엔카운터가 끝나지 않았다면 승리 처리
	if (bEncounterActive)
	{
		EndEncounter(EDuelEndResult::Victory);
	}
}

void ADuelEncounterManager::HandlePlayerDeathStarted(ABaseCharacter* DeadCharacter)
{
	if (!bEncounterActive)
	{
		return;
	}

	if (BossCharacter)
	{
		BossCharacter->StopBossBrain();
	}

	OnPlayerDeathStarted();
}

void ADuelEncounterManager::HandlePlayerDeathFinished(ABaseCharacter* DeadCharacter)
{
	if (!bEncounterActive)
	{
		return;
	}

	EndEncounter(EDuelEndResult::Defeat);
}