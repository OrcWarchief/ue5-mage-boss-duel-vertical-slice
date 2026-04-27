// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Characters/Core/BaseCharacter.h"
#include "MageBossCharacter.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class ETeleportPhase : uint8
{
	None UMETA(DisplayName = "None"),
	Startup UMETA(DisplayName = "Startup"),
	Hidden UMETA(DisplayName = "Hidden"),
	Recovery UMETA(DisplayName = "Recovery")
};

USTRUCT(BlueprintType)
struct FTeleportRuntime
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Teleport")
	EDodgeDirection Direction = EDodgeDirection::None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Teleport")
	ETeleportPhase Phase = ETeleportPhase::None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Teleport")
	FVector Destination = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Teleport")
	bool bMeshHidden = false;
};

UCLASS()
class MAGEBOSSDUEL_API AMageBossCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AMageBossCharacter();

	// ===== Target =====

	UFUNCTION(BlueprintCallable, Category = "Boss|Target")
	void SetCombatTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "Boss|Target")
	AActor* GetCombatTarget() const { return CurrentCombatTarget.Get(); }

	// ===== Teleport =====

	UFUNCTION(BlueprintPure, Category = "Skill|Teleport")
	bool CanStartTeleport(EDodgeDirection RequestedDirection = EDodgeDirection::None) const;

	UFUNCTION(BlueprintCallable, Category = "Skill|Teleport")
	bool TryStartTeleport(EDodgeDirection RequestedDirection = EDodgeDirection::None);

	/** AnimNotify: 사라지는 프레임에서 호출 */
	UFUNCTION(BlueprintCallable, Category = "Skill|Teleport|AnimNotify")
	void ExecuteTeleport();

	/** AnimNotify: 다시 보이는 프레임에서 호출 */
	UFUNCTION(BlueprintCallable, Category = "Skill|Teleport|AnimNotify")
	void ReappearTeleport();

	UFUNCTION(BlueprintPure, Category = "Skill|Teleport")
	bool IsTeleporting() const { return TeleportRuntime.Phase != ETeleportPhase::None; }

protected:
	// ===== BaseCharacter hooks =====

	virtual bool IsLockOnActive() const override;
	virtual AActor* GetCurrentLockOnTarget() const override;
	virtual AActor* GetLockOnTargetActor_Implementation() const override;

	virtual void OnHitReaction_Implementation() override;
	virtual void Die_Implementation() override;

	// ===== Target Runtime =====

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Boss|Target")
	TObjectPtr<AActor> CurrentCombatTarget = nullptr;

	// ===== Teleport Anim =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Anim")
	TObjectPtr<UAnimMontage> TeleportForwardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Anim")
	TObjectPtr<UAnimMontage> TeleportBackwardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Anim")
	TObjectPtr<UAnimMontage> TeleportLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Anim")
	TObjectPtr<UAnimMontage> TeleportRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Anim")
	TObjectPtr<UAnimMontage> TeleportForwardLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Anim")
	TObjectPtr<UAnimMontage> TeleportForwardRightMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Anim")
	TObjectPtr<UAnimMontage> TeleportBackwardLeftMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Anim")
	TObjectPtr<UAnimMontage> TeleportBackwardRightMontage = nullptr;

	// ===== Teleport Tuning =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float TeleportCooldown = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float TeleportDistance = 550.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float TeleportMinDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Trace", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float TeleportGroundTraceUp = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Trace", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float TeleportGroundTraceDown = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Trace", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float TeleportGroundOffset = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Trace", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TeleportMinGroundNormalZ = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Trace")
	TEnumAsByte<ECollisionChannel> TeleportGroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|Trace")
	TEnumAsByte<ECollisionChannel> TeleportOverlapChannel = ECC_Pawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|AI", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float TeleportNearDistance = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport|AI", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float TeleportFarDistance = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport")
	bool bFaceTargetOnTeleportStart = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Teleport")
	bool bFaceTargetOnReappear = true;

	// ===== Teleport Runtime =====

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Teleport|Runtime")
	FTeleportRuntime TeleportRuntime;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Teleport|Runtime")
	float LastTeleportTime = -9999.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Teleport|Runtime")
	EDodgeDirection LastTeleportDirection = EDodgeDirection::None;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveTeleportMontage = nullptr;

	EDodgeDirection ChooseTeleportDirectionForAI() const;
	UAnimMontage* GetTeleportMontage(EDodgeDirection Direction) const;

	void BeginTeleport(UAnimMontage* MontageToPlay, EDodgeDirection Direction, const FVector& Destination);
	void EndTeleport();
	void CancelTeleport(bool bRestoreNeutralState, bool bRestoreMovement);

	void OnTeleportMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	bool FindTeleportDestination(
		EDodgeDirection RequestedDirection,
		FVector& OutLocation,
		EDodgeDirection& OutResolvedDirection
	) const;

	FVector TeleportDirectionToWorld(EDodgeDirection Direction) const;
	FRotator MakeFacingRotationAtLocation(const FVector& WorldLocation) const;

	void SetTeleportHiddenState(bool bHidden);
	void FreezeMovementForTeleport();
	void RestoreMovementAfterTeleport();
};