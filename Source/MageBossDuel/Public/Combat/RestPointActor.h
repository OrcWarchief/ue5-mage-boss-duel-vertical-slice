// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RestPointActor.generated.h"

class APawn;
class USceneComponent;

UCLASS()
class MAGEBOSSDUEL_API ARestPointActor : public AActor
{
	GENERATED_BODY()
	
public:
	ARestPointActor();

	UFUNCTION(BlueprintCallable, Category = "Rest Point")
	void ActivateRestPoint(APawn* ActivatingPawn);

	UFUNCTION(BlueprintPure, Category = "Rest Point")
	FTransform GetRespawnTransform() const;

	UFUNCTION(BlueprintPure, Category = "Rest Point")
	FName GetRestPointId() const { return RestPointId; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RespawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest Point")
	FName RestPointId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest Point")
	bool bSetAsDefaultOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest Point|Debug")
	bool bDrawDebugRespawnPoint = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "Rest Point")
	void OnRestPointActivated(APawn* ActivatingPawn);

private:
	FName ResolveRestPointId() const;

};
