// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Core/BaseCharacter.h"
#include "InputActionValue.h"
#include "PlayerCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;

/**
 * 
 */
UCLASS()
class MAGEBOSSDUEL_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
public:
	APlayerCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_LockOn;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleLockOn(const FInputActionValue& Value);
	virtual void Jump() override;

	virtual AActor* GetLockOnTargetActor_Implementation() const override;

private:
	// Camera
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// Lock On
	UPROPERTY(VisibleInstanceOnly, Category = "LockOn|Runtime")
	bool bLockOnActive = false;

	UPROPERTY(VisibleInstanceOnly, Category = "LockOn|Runtime")
	TObjectPtr<AActor> LockOnTarget;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn|Acquire")
	float LockOnMaxDistance = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn|Acquire")
	float LockOnMaxAngleDegrees = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn|Acquire")
	bool bLockOnRequireLineOfSight = true;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn|Acquire")
	TEnumAsByte<ECollisionChannel> LockOnVisibilityChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn|Drive")
	float LockOnInterpSpeed = 12.f;

	void StartLockOn(AActor* NewTarget);
	void StopLockOn();
	void UpdateLockOn(float DeltaTime);
};
