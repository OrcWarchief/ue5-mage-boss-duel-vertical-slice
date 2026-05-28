// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossRoomEntranceTrigger.generated.h"

class APlayerController;
class ABossRoomBoundaryBlocker;
class ADuelEncounterManager;
class APawn;
class UBoxComponent;
class UDuelScreenFadeWidget;
class USceneComponent;

UCLASS()
class MAGEBOSSDUEL_API ABossRoomEntranceTrigger : public AActor
{
	GENERATED_BODY()
	
public:
	ABossRoomEntranceTrigger();

	UFUNCTION(BlueprintCallable, Category = "Boss Room|Boundary")
	void SetBossRoomBoundariesBlocked(bool bBlocked);

protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Room")
	TObjectPtr<ADuelEncounterManager> EncounterManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Room")
	TObjectPtr<AActor> ArenaEntryPoint;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Boss Room|Boundary")
	TArray<TObjectPtr<ABossRoomBoundaryBlocker>> BoundaryBlockers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Room|Boundary")
	bool bBlockBoundariesOnEntry = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Room|Transition")
	TSubclassOf<UDuelScreenFadeWidget> ScreenFadeWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss Room|Transition", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float FadeInDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss Room|Transition", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float BlackHoldDuration = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss Room|Transition", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float FadeOutDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss Room|Transition")
	int32 ScreenFadeZOrder = 300;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Room")
	bool bTriggerOnlyOnce = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss Room")
	void OnBossRoomEntryStarted(APawn* PlayerPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss Room")
	void OnBossRoomEntryFinished(APawn* PlayerPawn);

private:
	UPROPERTY()
	TObjectPtr<UDuelScreenFadeWidget> ActiveFadeWidget = nullptr;

	TWeakObjectPtr<APawn> PendingPawn;
	TWeakObjectPtr<APlayerController> PendingPlayerController;

	FTimerHandle TeleportTimerHandle;
	FTimerHandle FadeOutTimerHandle;
	FTimerHandle FinishTimerHandle;

	bool bEntryInProgress = false;
	bool bHasTriggered = false;

	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void BeginBossRoomEntry(APawn* PlayerPawn);
	void TeleportPlayerToArenaEntry();
	void BeginFadeOut();
	void FinishBossRoomEntry();

	void SetPlayerTransitionInputLocked(APlayerController* PlayerController, bool bLocked) const;
};
