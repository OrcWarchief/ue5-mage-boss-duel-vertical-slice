// BaseCharacter.h
// Shared character base: stats(Health), state machine, and basic attack.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

// ===== Forward Decls =====
class ABaseMagicProjectile;
class UAnimMontage;
class USceneComponent;
class AActor;   // 이 헤더는 AActor의 내부 구현에 의존하지 않음!

/** 간단 상태 머신(게임플레이 게이트 용). Dead는 terminal로 */
UENUM(BlueprintType)
enum class ECharacterState : uint8
{
    Idle      UMETA(DisplayName = "Idle"),
    Moving    UMETA(DisplayName = "Moving"),
    Attacking UMETA(DisplayName = "Attacking"),
    Hit       UMETA(DisplayName = "Hit"),
    Dead      UMETA(DisplayName = "Dead"),
};

/**
 * 플레이어/보스 공용 베이스.
 * - Health/Mana 관리(클램프) + 사망 처리
 * - 상태 전이(Idle/Moving/Attacking/Hit/Dead)
 * - 기본 공격 흐름:
 *  Start -> (AnimNotify: PerformBasicAttackHitCheck) -> End
 *   * 가까우면 근접 히트체크, 멀면 프로젝타일 발사
 */
UCLASS()
class MAGEBOSSDUEL_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
    ABaseCharacter();

    // ===== Health / Stats =====
    UFUNCTION(BlueprintCallable, Category = "Stats|Health")
    void SetHealth(float NewHealth);

    UFUNCTION(BlueprintCallable, Category = "Stats|Health")
    void ApplyDamage(float DamageAmount, ABaseCharacter* DamageCauser);

    UFUNCTION(BlueprintCallable, Category = "Stats|Health")
    void Heal(float HealAmount);

    UFUNCTION(BlueprintPure, Category = "Stats|Health")
    bool IsAlive() const;

        // ===== Combat =====
    UFUNCTION(BlueprintPure, Category = "Combat|Basic")
    bool CanBasicAttack() const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Basic")
    void StartBasicAttack();

    /** AnimNotify 타이밍에서 호출 */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat|Basic")
    void PerformBasicAttackHitCheck();

    UFUNCTION(BlueprintCallable, Category = "Combat|Basic")
    void EndBasicAttack();

    // ===== LifeCycle =====
    UFUNCTION(BlueprintCallable, Category = "LifeCycle")
    void OnDeathFinished();

    // ===== Targeting =====
    /** Hard Lock-on 타겟: 파생 클래스(Player)에서 락온 시스템이 있으면 override로 넘겨주기 */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Targeting")
    AActor* GetLockOnTargetActor() const;

    // ===== Hit =====
    UFUNCTION(BlueprintPure, Category = "Combat|Hit")
    bool CanBeInterrupted() const;

protected:
    virtual void BeginPlay() override;

    // ===== Hook =====
     /** 스탯 초기화 훅(CurrentHealth = MaxHealth, 이동속도 적용 등). */
    UFUNCTION(BlueprintNativeEvent, Category = "Stats")
    void InitializeStats();

    /** 사망 처리 훅(Dead 전환, 이동/충돌/입력/AI 비활성화 등). */
    UFUNCTION(BlueprintNativeEvent, Category = "LifeCycle")
    void Die();

    /** 피격 리액션 훅(상태=Hit, 몽타주/경직 등). */
    UFUNCTION(BlueprintNativeEvent, Category = "Combat|Hit")
    void OnHitReaction();

    /** 상태 전이(중앙집중). Dead는 되돌리지 않음. */
    UFUNCTION(BlueprintCallable, Category = "State", meta = (BlueprintProtected = "true"))
    void SetCharacterState(ECharacterState NewState);

    // ===== Targeting Helpers (3-tier) =====
    /** Hard -> Soft -> nullptr(FreeAim) */
    AActor* ResolveBasicAttackTarget() const;

    /** Soft Lock-on: 전방 Cone 내에서 가장 적절한 적 */
    AActor* FindSoftLockTarget(const FVector& ViewLocation, const FVector& ViewForward) const;

    /** 타겟을 어디로 조준할지(기본: Actor bounds center) */
    FVector GetTargetAimLocation(const AActor* TargetActor) const;

    /** Controller의 시점(카메라) 가져오기: Player/AI 공통 */
    void GetControllerViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

    /** 프로젝타일 발사(타겟이 있으면 그 방향, 없으면 Free Aim) */
    void FireBasicAttackProjectile(AActor* TargetActor);

    // ===== Mana Helpers =====
    bool HasEnoughMana(float Cost) const;
    bool TryConsumeMana(float Cost);


protected:
    // ===== Tuning (파생 클래스/디폴트에서 조정) =====
    /** 최대 Health (>= 1). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|HP", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float MaxHealth = 100.f;

    /** 최대 Mana (>= 0). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Mana", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float MaxMana = 100.f;

    /** 걷기 속도 (cm/s). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm/s"))
    float WalkSpeed = 200.f;

    /** 달리기 속도 (cm/s). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm/s"))
    float RunSpeed = 400.f;

    /** 회전 속도(보간용) (deg/s). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Rotation", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "deg/s"))
    float RotationSpeed = 720.f;

    /** 기본 공격 데미지. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Basic", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float BaseAttackDamage = 10.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Basic|Casting")
    UAnimMontage* BasicAttackMontage = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Basic|Casting", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float BasicAttackManaCost = 5.f;

    /** 기본 공격 쿨다운 (s). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Basic", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
    float AttackCooldown = 0.5f;

    // ===== Projectile (Far Attack) =====
    /** 멀리 있을 때 사용할 발사체 클래스 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Basic|Projectile")
    TSubclassOf<ABaseMagicProjectile> BasicAttackProjectileClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Basic|Projectile")
    FName BasicAttackMuzzleSocketName = TEXT("Muzzle");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Basic|Projectile")
    FVector BasicAttackMuzzleOffset = FVector::ZeroVector;

    // ===== Soft Lock-on 자동 타겟 =====
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|SoftLock", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
    float SoftLockMaxDistance = 2000.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|SoftLock", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0", Units = "deg"))
    float SoftLockMaxAngleDegrees = 25.f;

    /** 벽 뒤에 있는 적은 제거 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|SoftLock")
    bool bSoftLockRequireLineOfSight = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|SoftLock")
    TEnumAsByte<ECollisionChannel> SoftLockVisibilityChannel = ECC_Visibility;

private:
    // ===== Runtime (직접 수정 금지, ReadOnly만) =====
    /** 현재 Health. [0..MaxHealth] */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats|HP", meta = (AllowPrivateAccess = "true"))
    float CurrentHealth = 0.f;

    /** 현재 Mana. [0..MaxMana] */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats|Mana", meta = (AllowPrivateAccess = "true"))
    float CurrentMana = 0.f;

    /** 달리기 상태(속도 적용은 CharacterMovement 쪽에서). */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Movement|Runtime", meta = (AllowPrivateAccess = "true"))
    bool bIsRunning = false;

    /** 공격 진행 중인지(중복 Start 방지). */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Runtime", meta = (AllowPrivateAccess = "true"))
    bool bIsAttacking = false;

    /** 마지막 공격 시각(초). AttackCooldown 계산에 사용. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Runtime", meta = (AllowPrivateAccess = "true"))
    float LastAttackTime = -9999.f;

    /** 현재 상태(상태 머신). */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
    ECharacterState CurrentState = ECharacterState::Idle;
};
