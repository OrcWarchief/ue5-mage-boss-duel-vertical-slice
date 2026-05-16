// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/HitTypes.h"
#include "DelayedRuneProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class ABaseCharacter;

UCLASS()
class MAGEBOSSDUEL_API ADelayedRuneProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	ADelayedRuneProjectile();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Skill|Rune")
	void InitializeRune(
		ABaseCharacter* InDamageCauser,
		AActor* InTarget,
		float InActivationDelay,
		float InAimYawOffsetDegrees
	);

	UFUNCTION(BlueprintCallable, Category = "Skill|Rune")
	void ActivateRune();

	UFUNCTION(BlueprintCallable, Category = "Skill|Rune")
	void CancelRune();

	UFUNCTION(BlueprintCallable, Category = "Skill|Rune")
	void SetHitPayload(const FHitPayload& NewHitPayload);

	UFUNCTION(BlueprintPure, Category = "Skill|Rune")
	bool IsActivated() const { return bActivated; }

	UFUNCTION(BlueprintPure, Category = "Skill|Rune")
	bool HasImpacted() const { return bHasImpacted; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Rune|Hit")
	FHitPayload HitPayload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Rune|Move", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm/s"))
	float ProjectileSpeed = 1250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Rune|Aim", meta = (Units = "cm"))
	float AimHeightOffset = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Rune|Aim", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float TargetLeadSeconds = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Rune|Life", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float LifeSpanAfterActivation = 3.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Rune|Runtime")
	bool bActivated = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Rune|Runtime")
	bool bHasImpacted = false;

	UFUNCTION()
	void OnProjectileBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|Rune")
	void OnRuneInitialized(float InActivationDelay);

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|Rune")
	void OnRuneActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|Rune")
	void OnRuneImpacted(const FVector& ImpactLocation, AActor* HitActor);

private:
	TWeakObjectPtr<ABaseCharacter> DamageCauser;
	TWeakObjectPtr<AActor> TargetActor;

	FTimerHandle ActivationTimerHandle;

	float ActivationDelay = 0.0f;
	float AimYawOffsetDegrees = 0.0f;

	FVector GetAimDirection() const;
	void HandleImpact(AActor* HitActor, const FVector& ImpactLocation);
};
