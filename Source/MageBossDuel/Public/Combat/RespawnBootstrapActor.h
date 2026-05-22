// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RespawnBootstrapActor.generated.h"

UCLASS()
class MAGEBOSSDUEL_API ARespawnBootstrapActor : public AActor
{
	GENERATED_BODY()
	
public:
	ARespawnBootstrapActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	float RetryDelay = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	int32 MaxApplyAttempts = 20;

private:
	FTimerHandle ApplyRespawnTimerHandle;
	int32 ApplyAttempts = 0;

	void TryApplyPendingRespawn();
};
