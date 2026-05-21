// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MBDRespawnSubsystem.generated.h"

class APlayerController;

USTRUCT(BlueprintType)
struct FRespawnPointData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Respawn")
	bool bHasValidRestPoint = false;

	UPROPERTY(BlueprintReadOnly, Category = "Respawn")
	FName RestPointId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Respawn")
	FName LevelName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Respawn")
	FTransform RespawnTransform = FTransform::Identity;
};

UCLASS()
class MAGEBOSSDUEL_API UMBDRespawnSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void SetActiveRestPoint(
		FName RestPointId,
		FName LevelName,
		const FTransform& RespawnTransform
	);

	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void ClearActiveRestPoint();

	UFUNCTION(BlueprintPure, Category = "Respawn")
	bool HasActiveRestPoint() const { return ActiveRestPoint.bHasValidRestPoint; }

	UFUNCTION(BlueprintPure, Category = "Respawn")
	FRespawnPointData GetActiveRestPoint() const { return ActiveRestPoint; }

	UFUNCTION(BlueprintPure, Category = "Respawn")
	FName GetActiveRestPointId() const { return ActiveRestPoint.RestPointId; }

	UFUNCTION(BlueprintPure, Category = "Respawn")
	FName GetRespawnLevelName() const { return ActiveRestPoint.LevelName; }

	UFUNCTION(BlueprintPure, Category = "Respawn")
	FTransform GetRespawnTransform() const { return ActiveRestPoint.RespawnTransform; }

	UFUNCTION(BlueprintCallable, Category = "Respawn")
	bool RespawnPlayerAtActiveRestPoint(APlayerController* PlayerController);

private:
	UPROPERTY()
	FRespawnPointData ActiveRestPoint;
};
