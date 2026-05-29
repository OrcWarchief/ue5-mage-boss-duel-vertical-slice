// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossRoomBoundaryBlocker.generated.h"

class UBoxComponent;
class USceneComponent;

UCLASS()
class MAGEBOSSDUEL_API ABossRoomBoundaryBlocker : public AActor
{
	GENERATED_BODY()

public:
	ABossRoomBoundaryBlocker();

	UFUNCTION(BlueprintCallable, Category = "Boss Room|Boundary")
	void SetBlocked(bool bBlocked);

	UFUNCTION(BlueprintPure, Category = "Boss Room|Boundary")
	bool IsBlocked() const { return bIsBlocked; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Room|Boundary")
	bool bBlockedOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Room|Debug")
	bool bDrawDebugBlocker = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss Room|Boundary")
	void OnBoundaryBlockedChanged(bool bNewBlocked);

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Boss Room|Boundary")
	bool bIsBlocked = false;
};
