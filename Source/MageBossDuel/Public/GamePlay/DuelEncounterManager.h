// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DuelEncounterManager.generated.h"

class ABaseCharacter;
class AMageBossCharacter;
class UUserWidget;
class UBossEncounterHUDWidget;
class UDuelEndScreenWidget;

UENUM(BlueprintType)
enum class EDuelEndResult : uint8
{
	None UMETA(DisplayName = "None"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat")
};

UCLASS()
class MAGEBOSSDUEL_API ADuelEncounterManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ADuelEncounterManager();

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void StartEncounter();

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void EndEncounter(EDuelEndResult Result);

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void RestartEncounter();

	UFUNCTION(BlueprintCallable, Category = "Duel|Respawn")
	bool RequestRespawnFromDefeat();

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void SetPlayerInputLocked(bool bLocked, bool bLockLook);

	UFUNCTION(BlueprintPure, Category = "Duel")
	bool IsEncounterActive() const { return bEncounterActive; }

	UFUNCTION(BlueprintPure, Category = "Duel")
	bool HasEncounterEnded() const { return bEndFlowStarted; }

	UFUNCTION(BlueprintPure, Category = "Duel")
	EDuelEndResult GetCurrentEndResult() const { return CurrentEndResult; }

	UFUNCTION(BlueprintPure, Category = "Duel|References")
	ABaseCharacter* GetEncounterPlayer() const { return PlayerCharacter; }

	UFUNCTION(BlueprintPure, Category = "Duel|References")
	AMageBossCharacter* GetEncounterBoss() const { return BossCharacter; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Duel|References")
	TObjectPtr<ABaseCharacter> PlayerCharacter = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Duel|References")
	TObjectPtr<AMageBossCharacter> BossCharacter = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel")
	bool bAutoFindReferences = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel")
	bool bAutoStartEncounter = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel")
	bool bLockPlayerInputOnEnd = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel")
	bool bAllowCameraLookAfterVictory = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel")
	bool bDestroyRemainingBossSkillActorsOnBossDeath = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel")
	FName BossSkillActorTag = TEXT("BossSkillActor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|Timing", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float VictoryScreenDelay = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|Timing", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float DefeatScreenDelay = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|UI")
	TSubclassOf<UUserWidget> VictoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|UI")
	TSubclassOf<UDuelEndScreenWidget> DefeatWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|UI")
	TSubclassOf<UBossEncounterHUDWidget> BossEncounterHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|UI")
	int32 BossEncounterHUDZOrder = 50;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|UI", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float BossEncounterHUDHideRemoveDelay = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|UI")
	int32 EndWidgetZOrder = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duel|Restart")
	FName RestartMapName = NAME_None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Duel|Runtime")
	bool bEncounterActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Duel|Runtime")
	bool bEndFlowStarted = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Duel|Runtime")
	EDuelEndResult CurrentEndResult = EDuelEndResult::None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Duel|Runtime")
	TObjectPtr<UUserWidget> ActiveEndWidget = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Duel|Runtime")
	TObjectPtr<UBossEncounterHUDWidget> ActiveBossEncounterHUDWidget = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category = "Duel")
	void OnEncounterStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Duel")
	void OnBossDeathStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Duel")
	void OnPlayerDeathStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Duel")
	void OnVictoryFlowStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Duel")
	void OnDefeatFlowStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Duel")
	void OnEndScreenReady(EDuelEndResult Result);

	UFUNCTION(BlueprintImplementableEvent, Category = "Duel|Respawn")
	void OnRespawnFromDefeatSucceeded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Duel|Respawn")
	void OnRespawnFromDefeatFailed();

private:
	FTimerHandle EndScreenTimerHandle;
	FTimerHandle BossEncounterHUDRemoveTimerHandle;

	UFUNCTION()
	void HandleBossDeathStarted(ABaseCharacter* DeadCharacter);

	UFUNCTION()
	void HandleBossDeathFinished(ABaseCharacter* DeadCharacter);

	UFUNCTION()
	void HandlePlayerDeathStarted(ABaseCharacter* DeadCharacter);

	UFUNCTION()
	void HandlePlayerDeathFinished(ABaseCharacter* DeadCharacter);

	void AutoFindReferences();
	void BindDeathEvents();
	void ShowEndScreen();
	void ShowBossEncounterHUD();
	void HideBossEncounterHUD();
	void RemoveBossEncounterHUD();
	void ClearBossEncounterHUD();
	void DestroyRemainingBossSkillActors();
};
