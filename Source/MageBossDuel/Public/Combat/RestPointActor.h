// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RestPointActor.generated.h"

class APawn;
class USceneComponent;
class USphereComponent;
class UPrimitiveComponent;

UCLASS()
class MAGEBOSSDUEL_API ARestPointActor : public AActor
{
	GENERATED_BODY()
	
public:
	ARestPointActor();

	UFUNCTION(BlueprintCallable, Category = "Rest Point")
	void ActivateRestPoint(APawn* ActivatingPawn);

	UFUNCTION(BlueprintCallable, Category = "Rest Point")
	bool TryActivateRestPoint(APawn* ActivatingPawn);

	UFUNCTION(BlueprintPure, Category = "Rest Point")
	bool CanActivateRestPoint(APawn* ActivatingPawn) const;

	UFUNCTION(BlueprintPure, Category = "Rest Point")
	FTransform GetRespawnTransform() const;

	UFUNCTION(BlueprintPure, Category = "Rest Point")
	FName GetRestPointId() const { return RestPointId; }

	UFUNCTION(BlueprintPure, Category = "Rest Point")
	APawn* GetFocusedPawn() const { return FocusedPawn.Get(); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RespawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest Point")
	FName RestPointId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest Point")
	bool bSetAsDefaultOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest Point|Interaction")
	bool bRequirePlayerOverlapForActivation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest Point|Debug")
	bool bDrawDebugRespawnPoint = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "Rest Point")
	void OnRestPointActivated(APawn* ActivatingPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Rest Point|Interaction")
	void OnRestPointFocusChanged(APawn* InFocusedPawn, bool bHasFocus);

	UFUNCTION(BlueprintImplementableEvent, Category = "Rest Point|Interaction")
	void OnRestPointActivationFailed(APawn* ActivatingPawn);

private:
	TWeakObjectPtr<APawn> FocusedPawn;

	FName ResolveRestPointId() const;

	UFUNCTION()
	void HandleInteractionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void HandleInteractionEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

};
