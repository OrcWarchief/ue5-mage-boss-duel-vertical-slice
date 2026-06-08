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
class UAnimMontage;

UENUM(BlueprintType)
enum class EPlayerCombatMode : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Staff  UMETA(DisplayName = "Staff")
};

/**
 * 
 */
UCLASS()
class MAGEBOSSDUEL_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
public:
	//debug
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Charged Shot|Debug")
	bool bDrawChargedShotDebug = true;
	void DrawChargedShotDebug() const;
	//debug

	APlayerCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	//===== Staff Mode =====
	UFUNCTION(BlueprintCallable, Category = "Equipment|CombatMode")
	void SetCombatMode(EPlayerCombatMode NewCombatMode);

	UFUNCTION(BlueprintCallable, Category = "Equipment|Staff")
	void SetStaffMode(bool bNewStaffMode);

	UFUNCTION(BlueprintCallable, Category = "Equipment|Staff")
	bool CanStartStaffEquip() const;

	UFUNCTION(BlueprintCallable, Category = "Equipment|Staff")
	void StartStaffEquip();

	UFUNCTION(BlueprintCallable, Category = "Equipment|Staff")
	void SetStaffWeaponVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Equipment|CombatMode")
	EPlayerCombatMode GetCombatMode() const { return CombatMode; }

	UFUNCTION(BlueprintPure, Category = "Equipment|Staff")
	bool IsStaffMode() const { return CombatMode == EPlayerCombatMode::Staff; }

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_TargetSwitchX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Equip;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Dodge;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_BasicAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_ChargedAttack = nullptr;

	FVector2D MovementVector = FVector2D::ZeroVector;

	void Move(const FInputActionValue& Value);
	void OnMoveReleased(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleLockOn(const FInputActionValue& Value);
	virtual void Jump() override;
	void Equip(const FInputActionValue& Value);
	void Dodge(const FInputActionValue& Value);
	void BasicAttack(const FInputActionValue& Value);

	void StartChargedAttack(const FInputActionValue& Value);
	void UpdateChargedAttack(const FInputActionValue& Value);
	void ReleaseChargedAttack(const FInputActionValue& Value);
	void CancelChargedAttack(const FInputActionValue& Value);

	bool CanStartChargedAttack() const;
	void CancelChargedAttackInternal();

	virtual AActor* GetLockOnTargetActor_Implementation() const override;

	virtual bool IsLockOnActive() const override { return bLockOnActive; }
	virtual AActor* GetCurrentLockOnTarget() const override { return LockOnTarget.Get(); }

	virtual EDodgeDirection ResolveDodgeDirection(
		const FVector2D& MoveInput,
		bool bHasDirectionalInput
	) const override;

	virtual UAnimMontage* ResolveDodgeMontage(
		const FVector2D& MoveInput,
		EDodgeDirection Direction,
		bool bHasDirectionalInput
	) const override;

	// ===== Charged Attack Runtime =====

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Charged Attack")
	bool bIsChargingAttack = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Charged Attack")
	float CurrentChargedAttackTime = 0.0f;

	// ===== Charged Magic Shot Tuning =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Charged Attack", meta = (ClampMin = "0.0"))
	float MinChargedAttackTime = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Charged Attack", meta = (ClampMin = "0.1"))
	float MaxChargedAttackTime = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Charged Attack", meta = (ClampMin = "0.0"))
	float ChargedAttackManaCost = 25.0f;

	// ===== Charged Magic Shot Projectile =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Charged Shot")
	TSubclassOf<ABaseMagicProjectile> ChargedAttackProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Charged Shot")
	FHitPayload MinChargedAttackHitPayload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Charged Shot")
	FHitPayload MaxChargedAttackHitPayload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Charged Shot", meta = (ClampMin = "0.0"))
	float ChargedAttackSpawnForwardOffset = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Charged Shot", meta = (ClampMin = "0.0"))
	float ChargedAttackSpawnUpOffset = 60.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Charged Shot")
	void OnChargedAttackFired(float ChargeRatio);

	void FireChargedAttack(float ChargeRatio);
	FHitPayload BuildChargedAttackPayload(float ChargeRatio) const;
	FTransform GetChargedAttackSpawnTransform() const;

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

	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bEnableLockOnDebug = false;

	void StartLockOn(AActor* NewTarget);
	void StopLockOn();
	void UpdateLockOn(float DeltaTime);

	// Switch LockOn
	UPROPERTY(EditDefaultsOnly, Category = "LockOn|Switch")
	float TargetSwitchCooldown = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn|Switch")
	float TargetSwitchMinNormDx = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn|Switch")
	float TargetSwitchVerticalWeight = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn|Switch")
	float TargetSwitchDistanceWeight = 0.0005f;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn|Switch")
	float TargetSwitchMaxAngleDegree = 90.f;

	bool bTargetSwitchReady = true;
	float LastTargetSwitchTime = -9999.f;

	UFUNCTION()
	void OnTargetSwitchX(const FInputActionValue& Value);

	UFUNCTION()
	void OnTargetSwitchXReleased(const FInputActionValue& Value);

	bool TrySwitchLockOnTarget(int32 DirectionSign);
	AActor* FindSwitchTarget(int32 DirectionSign) const;

	// ===== Combat Mode =====

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment|Runtime", meta = (AllowPrivateAccess = "true"))
	EPlayerCombatMode CombatMode = EPlayerCombatMode::Normal;

	// ===== Staff Mode =====

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Staff", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> StaffWeaponMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Staff|Anim", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StaffEquipMontage = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment|Runtime", meta = (AllowPrivateAccess = "true"))
	bool bStaffEquipInProgress = false;

	UFUNCTION()
	void OnStaffEquipMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// ===== Staff Dodge Anim =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Staff", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StaffDodgeForwardRollMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Staff", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StaffDodgeBackwardRollMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Staff", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StaffDodgeLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Staff", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StaffDodgeRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Staff", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StaffDodgeForwardLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Staff", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StaffDodgeForwardRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Staff", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StaffDodgeBackwardLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Staff", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StaffDodgeBackwardRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge|Staff", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> StaffDodgeNeutralBackstepMontage = nullptr;

	UAnimMontage* GetStaffDodgeMontage(EDodgeDirection Direction) const;
};
