// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/HitTypes.h"
#include "RunePrisonBeamSegment.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ABaseCharacter;

UCLASS()
class MAGEBOSSDUEL_API ARunePrisonBeamSegment : public AActor
{
	GENERATED_BODY()

public:
	ARunePrisonBeamSegment();

	UFUNCTION(BlueprintCallable, Category = "Skill|RunePrison")
	void InitializeSegment(
		ABaseCharacter* InDamageCauser,
		const FVector& StartLocation,
		const FVector& EndLocation,
		float InBeamThickness,
		float InBeamHeight,
		const FHitPayload& InHitPayload
	);

	UFUNCTION(BlueprintCallable, Category = "Skill|RunePrison")
	void ActivateSegment();

	UFUNCTION(BlueprintPure, Category = "Skill|RunePrison")
	bool IsSegmentActive() const { return bSegmentActive; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|RunePrison")
	bool bSegmentActive = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|RunePrison|Debug")
	bool bDrawDebugSegment = false;

	UFUNCTION()
	void OnSegmentBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|RunePrison")
	void OnSegmentInitialized(
		const FVector& StartLocation,
		const FVector& EndLocation,
		float BeamThickness,
		float BeamHeight
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|RunePrison")
	void OnSegmentActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|RunePrison")
	void OnSegmentHitActor(AActor* HitActor);

private:
	TWeakObjectPtr<ABaseCharacter> DamageCauser;

	FHitPayload HitPayload;

	TSet<TWeakObjectPtr<AActor>> DamagedActors;

	bool IsIgnoredActor(AActor* Actor) const;
	void ApplyDamageToActor(AActor* Actor);
};