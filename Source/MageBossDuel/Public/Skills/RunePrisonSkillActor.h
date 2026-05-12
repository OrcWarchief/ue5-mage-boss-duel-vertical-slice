// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/HitTypes.h"
#include "RunePrisonSkillActor.generated.h"

class USceneComponent;
class ABaseCharacter;
class ARunePrisonBeamSegment;

UCLASS()
class MAGEBOSSDUEL_API ARunePrisonSkillActor : public AActor
{
	GENERATED_BODY()

public:
	ARunePrisonSkillActor();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Skill|RunePrison")
	void InitializePrison(
		ABaseCharacter* InDamageCauser,
		AActor* InTargetActor,
		const FVector& InPrisonCenter
	);

	UFUNCTION(BlueprintPure, Category = "Skill|RunePrison")
	bool IsPrisonActive() const { return bPrisonActive; }

	UFUNCTION(BlueprintPure, Category = "Skill|RunePrison")
	bool HasFinalBlastTriggered() const { return bFinalBlastTriggered; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	// ===== Pattern Shape =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Shape", meta = (ClampMin = "3", UIMin = "3"))
	int32 RuneCount = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Shape", meta = (ClampMin = "100.0", UIMin = "100.0", Units = "cm"))
	float PrisonRadius = 430.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Shape", meta = (ClampMin = "1.0", UIMin = "1.0", Units = "cm"))
	float BeamThickness = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Shape", meta = (ClampMin = "10.0", UIMin = "10.0", Units = "cm"))
	float BeamHeight = 160.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Shape")
	bool bOpenOneGap = true;

	// ===== Timing =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Timing", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float TelegraphDuration = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Timing", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float ActiveDurationBeforeFinalBlast = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Timing", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float CleanupDelayAfterFinalBlast = 0.15f;

	// ===== Damage =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Hit")
	FHitPayload BeamHitPayload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Hit")
	FHitPayload FinalBlastPayload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Hit", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float FinalBlastRadius = 360.0f;

	// ===== Spawn =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison")
	TSubclassOf<ARunePrisonBeamSegment> BeamSegmentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Trace")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Trace", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float GroundTraceUp = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Trace", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float GroundTraceDown = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Debug")
	bool bDrawDebug = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Runtime")
	bool bPrisonActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Runtime")
	bool bFinalBlastTriggered = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Runtime")
	int32 OpenGapIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Runtime")
	FVector PrisonCenter = FVector::ZeroVector;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ARunePrisonBeamSegment>> ActiveSegments;

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|RunePrison")
	void OnPrisonTelegraphStarted(
		const TArray<FVector>& InAnchorLocations,
		int32 InOpenGapIndex,
		float InTelegraphDuration
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|RunePrison")
	void OnPrisonActivated(
		const TArray<FVector>& InAnchorLocations,
		int32 InOpenGapIndex
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|RunePrison")
	void OnPrisonFinalBlast(
		const FVector& BlastOrigin,
		float BlastRadius
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|RunePrison")
	void OnPrisonFinished();

private:
	TWeakObjectPtr<ABaseCharacter> DamageCauser;
	TWeakObjectPtr<AActor> TargetActor;

	FTimerHandle ActivateTimerHandle;
	FTimerHandle FinalBlastTimerHandle;
	FTimerHandle CleanupTimerHandle;

	TArray<FVector> AnchorLocations;

	void ActivatePrison();
	void TriggerFinalBlast();
	void CleanupPrison();

	void BuildAnchorLocations();
	bool ProjectPointToGround(const FVector& SourcePoint, FVector& OutGroundPoint) const;

	void SpawnBeamSegments();
	void ApplyFinalBlastDamage();

	bool IsIgnoredActor(AActor* Actor) const;
};