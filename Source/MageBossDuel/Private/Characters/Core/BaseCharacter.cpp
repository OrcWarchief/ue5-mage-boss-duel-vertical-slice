// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Core/BaseCharacter.h"
#include "Projectiles/BaseMagicProjectile.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

namespace
{
	EDodgeDirection SelectEightWayDirectionFromAxes(const float ForwardValue, const float RightValue)
	{
		const float SignedAngleDegrees = FMath::UnwindDegrees(
			FMath::RadiansToDegrees(FMath::Atan2(RightValue, ForwardValue)));

		if (SignedAngleDegrees > -22.5f && SignedAngleDegrees <= 22.5f)
		{
			return EDodgeDirection::Forward;
		}
		if (SignedAngleDegrees > 22.5f && SignedAngleDegrees <= 67.5f)
		{
			return EDodgeDirection::ForwardRight;
		}
		if (SignedAngleDegrees > 67.5f && SignedAngleDegrees <= 112.5f)
		{
			return EDodgeDirection::Right;
		}
		if (SignedAngleDegrees > 112.5f && SignedAngleDegrees <= 157.5f)
		{
			return EDodgeDirection::BackwardRight;
		}
		if (SignedAngleDegrees > 157.5f || SignedAngleDegrees <= -157.5f)
		{
			return EDodgeDirection::Backward;
		}
		if (SignedAngleDegrees > -157.5f && SignedAngleDegrees <= -112.5f)
		{
			return EDodgeDirection::BackwardLeft;
		}
		if (SignedAngleDegrees > -112.5f && SignedAngleDegrees <= -67.5f)
		{
			return EDodgeDirection::Left;
		}

		return EDodgeDirection::ForwardLeft;
	}

	EHitReactionType MaxHitReaction(EHitReactionType A, EHitReactionType B)
	{
		return static_cast<uint8>(A) >= static_cast<uint8>(B) ? A : B;
	}
}

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	LockOnAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("LockOnAnchor"));
	LockOnAnchor->SetupAttachment(GetRootComponent());
	LockOnAnchor->SetRelativeLocation(FVector(0.f, 0.f, 80.f));

	TargetHealthBarAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("TargetHealthBarAnchor"));
	TargetHealthBarAnchor->SetupAttachment(GetRootComponent());
	TargetHealthBarAnchor->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	// 컨트롤러 yaw를 캐릭터가 따라가게
	bUseControllerRotationYaw = true;
	// 이동 방향으로 자동 회전 끔
	GetCharacterMovement()->bOrientRotationToMovement = false; 
}

void ABaseCharacter::SetInvulnerable(bool bNewInvulnerable)
{
	bIsInvulnerable = bNewInvulnerable;
}

void ABaseCharacter::ReviveForRespawn()
{
	SetLifeSpan(0.f); // 기존에 사망으로 설정된 수명 제거

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitRecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(PoiseRestoreTimerHandle);
		World->GetTimerManager().ClearTimer(DeathFinishTimerHandle);
	}

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (ActiveDeathMontage)
		{

			FOnMontageEnded EmptyMontageEndedDelegate;
			AnimInstance->Montage_SetEndDelegate(EmptyMontageEndedDelegate, ActiveDeathMontage);
		}

		AnimInstance->StopAllMontages(0.05f);
	}

	ActiveDeathMontage = nullptr;

	bDeathSequenceStarted = false;
	bDeathSequenceFinished = false;

	bIsAttacking = false;
	bHasPerformedBasicAttackHit = false;
	bIsRunning = false;

	CurrentDodgeDirection = EDodgeDirection::None;
	CurrentHitReactionType = EHitReactionType::None;

	SetInvulnerable(false);
	
	// SetHealth/ Mana/ Poise()는 Dead에서 상태 변경 막음으로, 여기서 직접 설정.
	CurrentHealth = MaxHealth;
	CurrentMana = MaxMana;
	CurrentPoise = MaxPoise;

	// SetCharacterState()는	 Dead에서 상태 변경 막음으로, 여기서 직접 설정.
	CurrentState = ECharacterState::Idle;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->MaxWalkSpeed = WalkSpeed;
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	SetActorHiddenInGame(false);

	BroadcastHealthChanged();
	BroadcastManaChanged();

	OnRevivedForRespawn();
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeStats();
}

// ===== Stats =====
void ABaseCharacter::InitializeStats_Implementation()
{
	// Health, Mana 초기화
	CurrentHealth = MaxHealth;
	CurrentMana = MaxMana;
	CurrentPoise = MaxPoise;
	CurrentHitReactionType = EHitReactionType::None;

	// 전투/상태 초기화
	bIsAttacking = false;
	bHasPerformedBasicAttackHit = false;
	LastAttackTime = -9999.f;

	bDeathSequenceStarted = false;
	bDeathSequenceFinished = false;
	ActiveDeathMontage = nullptr;

	SetInvulnerable(false);
	SetCharacterState(ECharacterState::Idle);

	// 이동속도 초기화
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = WalkSpeed;
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	BroadcastHealthChanged();
	BroadcastManaChanged();
}

void ABaseCharacter::SetHealth(float NewHealth)
{
	if (CurrentState == ECharacterState::Dead) return;

	const float ClampedHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	CurrentHealth = ClampedHealth;

	BroadcastHealthChanged();
	
	if (CurrentHealth <= 0.f)
	{
		SetCharacterState(ECharacterState::Dead); // terminal 보장
	}
}

void ABaseCharacter::ApplyDamage(const FHitPayload& HitPayload, ABaseCharacter* DamageCauser)
{
	if (!IsAlive())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DAMAGE-FAIL] Target already dead"));
		return;
	}

	ApplyHitPayload(HitPayload, DamageCauser);
}

void ABaseCharacter::ApplyHitPayload(const FHitPayload& HitPayload, ABaseCharacter* DamageCauser)
{
	(void)DamageCauser; // TODO: distance/Knockback calculate

	if (!IsAlive())
	{
		return;
	}

	if (bIsInvulnerable)
	{
		return;
	}

	const bool bHasDamage = HitPayload.Damage > 0.0f;
	const bool bHasPoiseDamage = HitPayload.PoiseDamage > 0.0f;

	if (!bHasDamage && !bHasPoiseDamage && !HitPayload.bForceReaction)
	{
		return;
	}

	// ===== Damage =====

	if (bHasDamage)
	{
		// TODO: Defense, 나중에 FinalDamage 계산식으로 확장
		const float FinalDamage = HitPayload.Damage;
		const float NewHealth = CurrentHealth - FinalDamage;

		SetHealth(NewHealth);
	}

	// SetHealth에서 Dead로 바뀌었으면 HitReaction은 실행X
	if (!IsAlive())
	{
		return;
	}

	// ===== Poise =====

	bool bPoiseBroken = false;

	if (!HitPayload.bIgnorePoise && MaxPoise > 0.0f && bHasPoiseDamage)
	{
		CurrentPoise = FMath::Clamp(CurrentPoise - HitPayload.PoiseDamage, 0.0f, MaxPoise);
		bPoiseBroken = CurrentPoise <= 0.0f;
	}

	// ===== Reaction Resolve =====

	const EHitReactionType ResolvedReaction = ResolveHitReaction(HitPayload, bPoiseBroken);

	if (bPoiseBroken)
	{
		// poise break -> 즉시 poise를 회복
		// 한 번 깨진 뒤 영구적으로 계속 HeavyStagger가 나지 않음
		CurrentPoise = MaxPoise;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PoiseRestoreTimerHandle);
		}
	}
	else if (bHasPoiseDamage)
	{
		SchedulePoiseRestore();
	}

	if (ResolvedReaction != EHitReactionType::None)
	{
		CurrentHitReactionType = ResolvedReaction;
		OnHitReaction();
	}
}

void ABaseCharacter::Heal(float HealAmount)
{
	if (!IsAlive())
	{
		return;
	}

	if (HealAmount <= 0.f)
	{
		return;
	}
	const float NewHealth = CurrentHealth + HealAmount;
	SetHealth(NewHealth); // 내부에서 Clamp 처리
}

bool ABaseCharacter::IsAlive() const
{
	return CurrentState != ECharacterState::Dead;
}

// ===== Combat =====
bool ABaseCharacter::CanBasicAttack() const
{
	if (!IsAlive()) 
	{
		return false;
	}
	if (bIsAttacking)
	{
		return false;
	}
	if (!BasicAttackProjectileClass)
	{
		return false;
	}

	if (CurrentState != ECharacterState::Idle && CurrentState != ECharacterState::Moving)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	
	const float Now = World->GetTimeSeconds();
	const bool bCooldownDone = (Now - LastAttackTime) >= AttackCooldown;
	if (!bCooldownDone) { return false; }
	
	if (!HasEnoughMana(BasicAttackManaCost)) { return false; }

	return true;
}

void ABaseCharacter::StartBasicAttack()
{
	if (bEnableCombatDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartBasicAttack"));
	}
	if (!CanBasicAttack())
	{
		return;
	}

	bIsAttacking = true;
	bHasPerformedBasicAttackHit = false;
	SetCharacterState(ECharacterState::Attacking);

	if (UWorld* World = GetWorld())
	{
		LastAttackTime = World->GetTimeSeconds();
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && BasicAttackMontage)
	{
		const float PlayedLength = AnimInstance->Montage_Play(BasicAttackMontage, 1.0f);
		if (PlayedLength <= 0.f)
		{
			EndBasicAttack();
			return;
		}

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &ThisClass::OnBasicAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, BasicAttackMontage);
	}
	else
	{
		// 몽타주 없으면 즉시 발사 후 종료
		PerformBasicAttackHitCheck();
		EndBasicAttack();
	}
}

void ABaseCharacter::OnBasicAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndBasicAttack();
}

void ABaseCharacter::PerformBasicAttackHitCheck_Implementation()
{
	if (!IsAlive() || !bIsAttacking)
	{
		return;
	}

	if (bHasPerformedBasicAttackHit)
	{
		return;
	}

	bHasPerformedBasicAttackHit = true;

	if (!TryConsumeMana(BasicAttackManaCost))
	{
		// 마나 부족이면 발사 없이 공격 취소
		EndBasicAttack();
		return;
	}
	// 하드 락온 -> 소프트 락온 -> nullptr
	AActor* Target = ResolveBasicAttackTarget();
	FireBasicAttackProjectile(Target);
}

void ABaseCharacter::EndBasicAttack()
{
	if (bEnableCombatDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("End Basic Attack"));
	}

	if (!bIsAttacking)
	{
		return;
	}
	bIsAttacking = false;
	bHasPerformedBasicAttackHit = false;
	// 상태 복귀: 속도 기반으로 Idle/Moving
	const float Speed2D = GetVelocity().Size2D();
	SetCharacterState(Speed2D > 3.f ? ECharacterState::Moving : ECharacterState::Idle);
}

// ===== Targeting (3-tier) =====
AActor* ABaseCharacter::ResolveBasicAttackTarget() const
{
	// 하드 락온
	AActor* HardTarget = GetLockOnTargetActor();
	if (HardTarget && HardTarget != this)
	{
		return HardTarget;
	}
	
	// 소프트 락온
	FVector ViewLocation;
	FRotator ViewRotation;
	GetControllerViewPoint(ViewLocation, ViewRotation);
	FVector ViewForward = ViewRotation.Vector();
	AActor* SoftTarget = FindSoftLockTarget(ViewLocation, ViewForward);
	if (SoftTarget && SoftTarget != this)
	{
		return SoftTarget;
	}

	// 타겟 없음
	return nullptr;
}

AActor* ABaseCharacter::FindLockOnTarget(
	const FVector& ViewLoc, 
	const FVector& ViewForward, 
	const FVector& SearchCenter, 
	float MaxDistance, 
	float MaxAngleDegrees, 
	bool bRequireLineOfSight, 
	ECollisionChannel VisibilityChannel
) const
{
	UWorld* World = GetWorld();
	if (!World) { return nullptr; }

	TArray<FOverlapResult> Overlaps;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LockOnOverlap), false);
	QueryParams.AddIgnoredActor(this);

	const bool bAny = World->OverlapMultiByObjectType(
		Overlaps,
		SearchCenter,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(MaxDistance),
		QueryParams
	);

	if (bEnableCombatDebug)
	{
		DrawDebugSphere(World, SearchCenter, MaxDistance, 24, FColor::Cyan, false, 1.0f, 0, 1.0f);
	}

	if (!bAny) { return nullptr; }

	double BestScore = -DBL_MAX;
	AActor* BestTarget = nullptr;

	const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(MaxAngleDegrees));

	for (const FOverlapResult& R : Overlaps)
	{
		AActor* Candidate = R.GetActor();
		if (!IsValid(Candidate) || Candidate == this) { continue; }

		if (const ABaseCharacter* BaseChar = Cast<ABaseCharacter>(Candidate))
		{
			if (!BaseChar->IsAlive()) { continue; }
		}

		const FVector AimLoc = GetTargetAimLocation(Candidate);
		const FVector ToCandidate = AimLoc - ViewLoc;

		const float Dist = ToCandidate.Size();
		if (Dist <= KINDA_SMALL_NUMBER) { continue; }

		const FVector Dir = ToCandidate / Dist;
		const float Dot = FVector::DotProduct(ViewForward, Dir);

		// 전방 콘 모양 필터
		if (Dot < CosThreshold) { continue; }

		// LOS 필터 (벽 뒤 등 시야 에서 안보이는거 제외)
		if (bRequireLineOfSight)
		{
			FHitResult Hit;
			FCollisionQueryParams LoSParams(SCENE_QUERY_STAT(TargetAcquireLoS), true, this);

			const bool bHit = World->LineTraceSingleByChannel(
				Hit,
				ViewLoc,
				AimLoc,
				VisibilityChannel,
				LoSParams
			);

			if (bHit && Hit.GetActor() != Candidate)
			{
				continue;
			}
		}

		// 스코어: 정면에 가까울수록 +, 멀수록 -
		const double Score = (double)Dot * 1000.0 - (double)Dist;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

AActor* ABaseCharacter::FindSoftLockTarget(const FVector& ViewLocation, const FVector& ViewForward) const
{
	AActor* SoftTarget = FindLockOnTarget(
		ViewLocation,
		ViewForward,
		ViewLocation,
		SoftLockMaxDistance,
		SoftLockMaxAngleDegrees,
		bSoftLockRequireLineOfSight,
		SoftLockVisibilityChannel
	);

	return SoftTarget;
}

FVector ABaseCharacter::GetTargetAimLocation(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor)) { return FVector::ZeroVector; }

	if (const ABaseCharacter* BaseTarget = Cast<ABaseCharacter>(TargetActor))
	{
		return BaseTarget->GetLockOnWorldLocation();
	}

	FVector Origin, BoxExtent;
	TargetActor->GetActorBounds(true, Origin, BoxExtent);
	return Origin;
}

void ABaseCharacter::GetControllerViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	if (Controller)
	{
		Controller->GetPlayerViewPoint(OutLocation, OutRotation);
	}
	else
	{
		OutLocation = GetActorLocation();
		OutRotation = GetActorRotation();
	}
}

// ===== Projectile =====
void ABaseCharacter::FireBasicAttackProjectile(AActor* TargetActor)
{
	if (!BasicAttackProjectileClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();	// ShooterMeshComp

	// 1) 스폰 위치 머즐 소켓 우선
	FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 50.f + BasicAttackMuzzleOffset;

	if (MeshComp && MeshComp->DoesSocketExist(BasicAttackMuzzleSocketName))
	{
		// TODO: 머즐 소켓 이름 BasicAttackMuzzleSocketName 추가
		const FTransform MuzzleTransform = MeshComp->GetSocketTransform(BasicAttackMuzzleSocketName, RTS_World);
		SpawnLocation = MuzzleTransform.TransformPosition(BasicAttackMuzzleOffset);
	}

	// 2) 에임 회전: (Hard/Soft) 타겟이 있으면 타겟 방향, 없으면 카메라 방향
	FVector ViewLoc;
	FRotator ViewRot;
	GetControllerViewPoint(ViewLoc, ViewRot);

	FRotator SpawnRotation = ViewRot; // Free Aim 기본

	if (IsValid(TargetActor))
	{
		const FVector AimLoc = GetTargetAimLocation(TargetActor);
		const FVector Dir = (AimLoc - SpawnLocation).GetSafeNormal();
		if (!Dir.IsNearlyZero())
		{
			SpawnRotation = Dir.Rotation();
		}
	}

	// 3) Spawn
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 프로젝타일 유효 검사 후 발사
	ABaseMagicProjectile* SpawnedProjectile =
		World->SpawnActor<ABaseMagicProjectile>(
			BasicAttackProjectileClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParameters
		);

	if (SpawnedProjectile)
	{
		SpawnedProjectile->SetDamage(BaseAttackDamage);
	}
}

// ===== LifeCycle / Hit =====
void ABaseCharacter::OnDeathFinished()
{
	// 데스 몽타주 종료시점에서 호출하는 용도 2초 후 destroy
	SetLifeSpan(2.0f);
}

FVector ABaseCharacter::GetLockOnWorldLocation() const
{
	if (LockOnAnchor)
	{
		return LockOnAnchor->GetComponentLocation();
	}

	FVector Origin, BoxExtent;
	GetActorBounds(true, Origin, BoxExtent);
	return Origin;
}

FVector ABaseCharacter::GetTargetHealthBarWorldLocation() const
{
	if (TargetHealthBarAnchor)
	{
		return TargetHealthBarAnchor->GetComponentLocation();
	}
	FVector Origin, BoxExtent;
	GetActorBounds(true, Origin, BoxExtent);
	return Origin + FVector(0.f, 0.f, BoxExtent.Z + 20.f);
}

AActor* ABaseCharacter::GetLockOnTargetActor_Implementation() const
{
	// 기본: 락온 시스템 없으면 nullptr
	return nullptr;
}

bool ABaseCharacter::CanBeInterrupted() const
{
	// 아주 단순 버전 : 공격 중이면 인터럽트 가능
	// 슈퍼아머, Poise  확장?
	return IsAlive() && bIsAttacking;
}

bool ABaseCharacter::TryStartDodge(const FVector2D& MoveInput)
{
	if (!CanStartDodge()) { return false; }

	const bool bHasDirectionalInput = HasMeaningfulMoveInput(MoveInput);
	const EDodgeDirection Direction = ResolveDodgeDirection(MoveInput, bHasDirectionalInput);

	UAnimMontage* MontageToPlay = ResolveDodgeMontage(
		MoveInput,
		Direction,
		bHasDirectionalInput
	);
	if (!MontageToPlay)
	{
		return false;
	}

	BeginDodge(MontageToPlay, Direction);
	return true;
}

void ABaseCharacter::Die_Implementation()
{
	if (CurrentState != ECharacterState::Dead)
	{
		return;
	}

	if (bDeathSequenceStarted)
	{
		return;
	}

	bDeathSequenceStarted = true;
	bDeathSequenceFinished = false;

	// 상태 정리
	CurrentHitReactionType = EHitReactionType::None;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitRecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(PoiseRestoreTimerHandle);
		World->GetTimerManager().ClearTimer(DeathFinishTimerHandle);
	}

	bIsAttacking = false;
	bHasPerformedBasicAttackHit = false;

	CurrentHitReactionType = EHitReactionType::None;
	SetInvulnerable(false);
	SetCharacterState(ECharacterState::Dead);

	// 이동 정지
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}

	// 콜리전 비활성화
	if (bDisableCapsuleCollisionOnDeath)
	{
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

	if (AnimInstance && bStopAllMontagesBeforeDeathMontage)
	{
		AnimInstance->StopAllMontages(0.05f);
	}

	OnCharacterDeathStarted.Broadcast(this);

	if (AnimInstance && DeathMontage)
	{
		ActiveDeathMontage = DeathMontage;

		const float PlayLength = AnimInstance->Montage_Play(DeathMontage, 1.0f);

		if (PlayLength > 0.0f)
		{
			FOnMontageEnded MontageEndedDelegate;
			MontageEndedDelegate.BindUObject(
				this,
				&ABaseCharacter::OnDeathMontageEnded
			);

			AnimInstance->Montage_SetEndDelegate(
				MontageEndedDelegate,
				DeathMontage
			);

			return;
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (DeathFallbackDuration <= 0.0f)
		{
			FinishDeathSequence();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				DeathFinishTimerHandle,
				this,
				&ABaseCharacter::FinishDeathSequence,
				DeathFallbackDuration,
				false
			);
		}
	}
}

void ABaseCharacter::OnHitReaction_Implementation()
{
	const EHitReactionType ReactionType =
		CurrentHitReactionType == EHitReactionType::None
		? EHitReactionType::LightStagger
		: CurrentHitReactionType;

	if (bIsAttacking && CanBeInterrupted())
	{
		// 공격 강제 종료
		EndBasicAttack();
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}

	if (UAnimMontage* HitMontage = GetHitReactionMontage(ReactionType))
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Play(HitMontage, 1.0f);
		}
	}

	SetCharacterState(ECharacterState::Hit);
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitRecoveryTimerHandle);

		const float RecoveryDuration = GetHitRecoveryDuration(ReactionType);

		if (RecoveryDuration <= 0.0f)
		{
			OnHitRecoveryTimerElapsed();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				HitRecoveryTimerHandle,
				this,
				&ThisClass::OnHitRecoveryTimerElapsed,
				RecoveryDuration,
				false
			);
		}
	}
}

void ABaseCharacter::OnHitRecoveryTimerElapsed()
{
	if (!IsAlive())
	{
		return;
	}

	if (CurrentState != ECharacterState::Hit)
	{
		return;
	}

	CurrentHitReactionType = EHitReactionType::None;
	SetCharacterState(GetVelocity().Size2D() > 3.f ? ECharacterState::Moving : ECharacterState::Idle);
}

void ABaseCharacter::SetCharacterState(ECharacterState NewState)
{
	// 죽으면 다른 상태로 전이 불가능
	if (CurrentState == ECharacterState::Dead)
	{
		return;
	}

	if (CurrentState == ECharacterState::Hit && NewState != ECharacterState::Hit)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(HitRecoveryTimerHandle);
		}
	}
	
	CurrentState = NewState;

	// 상태에 따른 공통 제어
	switch (CurrentState)
	{
	case ECharacterState::Idle:
	case ECharacterState::Moving:
		break;

	case ECharacterState::Attacking:
		break;

	case ECharacterState::Hit:
		// TODO : 피격 중 공격 불가 처리
		break;

	case ECharacterState::Dead:
		Die();
		break;

	default:
		break;
	}
}

bool ABaseCharacter::CanEnterDodgeFromCurrentState() const
{
	return CurrentState == ECharacterState::Idle || CurrentState == ECharacterState::Moving;
}

void ABaseCharacter::OnDodgeStarted_StateHook()
{
	SetCharacterState(ECharacterState::Dodging);
}

void ABaseCharacter::OnDodgeEnded_StateHook()
{
	SetCharacterState(GetVelocity().Size2D() > 10.f
		? ECharacterState::Moving
		: ECharacterState::Idle);
}

bool ABaseCharacter::IsLockOnActive() const
{
	return false;
}

AActor* ABaseCharacter::GetCurrentLockOnTarget() const
{
	return nullptr;
}

// ===== Mana =====
bool ABaseCharacter::HasEnoughMana(float Cost) const
{
	if (Cost <= 0.f)
	{
		return true;
	}
	return CurrentMana >= Cost;
}

bool ABaseCharacter::TryConsumeMana(float Cost)
{
	if (Cost <= 0.f)
	{
		return true;
	}

	if (CurrentMana < Cost)
	{
		return false;
	}

	CurrentMana = FMath::Clamp(CurrentMana - Cost, 0.f, MaxMana);
	BroadcastManaChanged();

	return true;
}

void ABaseCharacter::BroadcastHealthChanged()
{
	const float Percent = MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, Percent);
}

void ABaseCharacter::BroadcastManaChanged()
{
	const float Percent = MaxMana > 0.f ? CurrentMana / MaxMana : 0.f;
	OnManaChanged.Broadcast(CurrentMana, MaxMana, Percent);
}

EHitReactionType ABaseCharacter::ResolveHitReaction(const FHitPayload& HitPayload, bool bPoiseBroken) const
{
	if (HitPayload.bForceReaction)
	{
		return HitPayload.ReactionType;
	}

	if (bPoiseBroken)
	{
		return MaxHitReaction(HitPayload.ReactionType, EHitReactionType::HeavyStagger);
	}

	if (HitPayload.bCanInterrupt && CanBeInterrupted())
	{
		return HitPayload.ReactionType;
	}

	return EHitReactionType::None;
}

float ABaseCharacter::GetHitRecoveryDuration(EHitReactionType ReactionType) const
{
	switch (ReactionType)
	{
	case EHitReactionType::LightStagger:
		return HitRecoveryDuration;
	case EHitReactionType::HeavyStagger:
		return HeavyHitRecoveryDuration;
	case EHitReactionType::Knockdown:
		return KnockdownRecoveryDuration;
	case EHitReactionType::None:
	default:
		return 0.0f;
	}
}

UAnimMontage* ABaseCharacter::GetHitReactionMontage(EHitReactionType ReactionType) const
{
	switch (ReactionType)
	{
	case EHitReactionType::LightStagger:
		return LightHitReactionMontage.Get();
	case EHitReactionType::HeavyStagger:
		return HeavyHitReactionMontage.Get();
	case EHitReactionType::Knockdown:
		return KnockdownMontage.Get();
	case EHitReactionType::None:
	default:
		return nullptr;
	}
}

void ABaseCharacter::SchedulePoiseRestore()
{
	if (MaxPoise <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(PoiseRestoreTimerHandle);

	if (PoiseRestoreDelay <= 0.0f)
	{
		RestorePoise();
		return;
	}

	World->GetTimerManager().SetTimer(
		PoiseRestoreTimerHandle,
		this,
		&ThisClass::RestorePoise,
		PoiseRestoreDelay,
		false
	);
}

void ABaseCharacter::RestorePoise()
{
	if (!IsAlive())
	{
		return;
	}

	CurrentPoise = MaxPoise;
}

bool ABaseCharacter::CanStartDodge() const
{
	if (CurrentState == ECharacterState::Dodging) { return false; }
	if (!CanEnterDodgeFromCurrentState()) { return false; }

	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) { return false; }
	if (!MoveComp->IsMovingOnGround()) { return false; }

	if (!GetMesh() || !GetMesh()->GetAnimInstance()) { return false; }

	return true;
}

bool ABaseCharacter::HasMeaningfulMoveInput(const FVector2D& MoveInput) const
{
	return MoveInput.SizeSquared() >= FMath::Square(MoveInputDeadZone);
}

FVector ABaseCharacter::GetDesiredMoveWorldDirection(const FVector2D& MoveInput) const
{
	if (!HasMeaningfulMoveInput(MoveInput))
	{
		return FVector::ZeroVector;
	}

	FVector ForwardDir = GetActorForwardVector();
	FVector RightDir = GetActorRightVector();

	if (Controller)
	{
		const FRotator ControlRot = Controller->GetControlRotation();
		const FRotator YawOnlyRot(0.f, ControlRot.Yaw, 0.f);

		ForwardDir = FRotationMatrix(YawOnlyRot).GetUnitAxis(EAxis::X);
		RightDir = FRotationMatrix(YawOnlyRot).GetUnitAxis(EAxis::Y);
	}

	FVector WorldDir = (ForwardDir * MoveInput.Y) + (RightDir * MoveInput.X);
	WorldDir.Z = 0;

	return WorldDir.GetSafeNormal();
}

void ABaseCharacter::FaceWorldDirection(const FVector& WorldDirection)
{
	FVector FlatDir = WorldDirection;
	FlatDir.Z = 0.f;

	if (FlatDir.IsNearlyZero())
	{
		return;
	}

	SetActorRotation(FlatDir.Rotation());
}

FVector ABaseCharacter::GetLockOnBasisForward() const
{
	if (IsLockOnActive())
	{
		if (AActor* LockTarget = GetCurrentLockOnTarget())
		{
			FVector ToTarget = LockTarget->GetActorLocation() - GetActorLocation();
			ToTarget.Z = 0.f;

			if (!ToTarget.IsNearlyZero())
			{
				return ToTarget.GetSafeNormal();
			}
		}
	}

	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.f;
	return Forward.GetSafeNormal();
}

FVector ABaseCharacter::GetLockOnBasisRight() const
{
	const FVector RefForward = GetLockOnBasisForward();

	FVector RefRight = FVector::CrossProduct(FVector::UpVector, RefForward);
	RefRight.Z = 0.f;

	if (RefRight.IsNearlyZero())
	{
		RefRight = GetActorRightVector();
		RefRight.Z = 0.f;
	}

	return RefRight.GetSafeNormal();
}

EDodgeDirection ABaseCharacter::SelectDodgeDirection(const FVector2D& MoveInput) const
{
	const bool bLockedOn = IsLockOnActive() && IsValid(GetCurrentLockOnTarget());

	if (!HasMeaningfulMoveInput(MoveInput))
	{
		return bLockedOn ? EDodgeDirection::Backward : EDodgeDirection::Forward;
	}

	const FVector DesiredWorldDir = GetDesiredMoveWorldDirection(MoveInput);
	if (DesiredWorldDir.IsNearlyZero())
	{
		return bLockedOn ? EDodgeDirection::Backward : EDodgeDirection::Forward;
	}
	
	const FVector RefForward = bLockedOn
		? GetLockOnBasisForward()
		: GetActorForwardVector().GetSafeNormal2D();
	const FVector RefRight = bLockedOn
		? GetLockOnBasisRight()
		: GetActorRightVector().GetSafeNormal2D();
	
	const float ForwardValue = FVector::DotProduct(DesiredWorldDir, RefForward);
	const float RightValue = FVector::DotProduct(DesiredWorldDir, RefRight);

	return SelectEightWayDirectionFromAxes(ForwardValue, RightValue);
}

EDodgeDirection ABaseCharacter::ResolveDodgeDirection(const FVector2D& MoveInput, bool bHasDirectionalInput) const
{
	(void)bHasDirectionalInput;

	return SelectDodgeDirection(MoveInput);
}

UAnimMontage* ABaseCharacter::ResolveDodgeMontage(const FVector2D& MoveInput, EDodgeDirection Direction, bool bHasDirectionalInput) const
{
	(void)MoveInput;

	if (!bHasDirectionalInput && DodgeNeutralBackstepMontage)
	{
		return DodgeNeutralBackstepMontage.Get();
	}

	return GetDodgeMontage(Direction);
}

UAnimMontage* ABaseCharacter::GetDodgeMontage(EDodgeDirection Direction) const
{
	switch (Direction)
	{
	case EDodgeDirection::Forward:
		return DodgeForwardRollMontage.Get();
	case EDodgeDirection::Backward:
		return DodgeBackwardRollMontage.Get();
	case EDodgeDirection::Left:
		return DodgeLeftMontage.Get();
	case EDodgeDirection::Right:
		return DodgeRightMontage.Get();
	case EDodgeDirection::ForwardLeft:
		return DodgeForwardLeftMontage.Get();
	case EDodgeDirection::ForwardRight:
		return DodgeForwardRightMontage.Get();
	case EDodgeDirection::BackwardLeft:
		return DodgeBackwardLeftMontage.Get();
	case EDodgeDirection::BackwardRight:
		return DodgeBackwardRightMontage.Get();
	default:
		return nullptr;
	}
}

void ABaseCharacter::BeginDodge(UAnimMontage* MontageToPlay, EDodgeDirection Direction)
{
	UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	
	if (!AnimInst || !MoveComp || !MontageToPlay) { return; }

	UE_LOG(LogTemp, Warning, TEXT("BeginDodge | Montage=%s | Direction=%d"),
		*GetNameSafe(MontageToPlay),
		static_cast<int32>(Direction));

	CurrentDodgeDirection = Direction;
	OnDodgeStarted_StateHook();

	MoveComp->StopMovementImmediately();

	bSavedOrientRotationToMovement = MoveComp->bOrientRotationToMovement;
	bSavedUseControllerDesiredRotation = MoveComp->bUseControllerDesiredRotation;
	bSavedUseControllerRotationYaw = bUseControllerRotationYaw;

	MoveComp->bOrientRotationToMovement = false;
	MoveComp->bUseControllerDesiredRotation = false;
	bUseControllerRotationYaw = false;

	if (IsLockOnActive())
	{
		FaceWorldDirection(GetLockOnBasisForward());
	}

	const float PlayedLength = AnimInst->Montage_Play(MontageToPlay, 1.0f);
	if (PlayedLength <= 0.f)
	{
		EndDodge();
		return;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &ThisClass::OnDodgeMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, MontageToPlay);
}

void ABaseCharacter::EndDodge()
{
	UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

	UE_LOG(LogTemp, Warning, TEXT("EndDodge called | State=%d | Dir=%d | ActiveMontage=%s"),
		static_cast<int32>(CurrentState),
		static_cast<int32>(CurrentDodgeDirection),
		*GetNameSafe(AnimInst ? AnimInst->GetCurrentActiveMontage() : nullptr));

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		MoveComp->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
	}

	bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
	CurrentDodgeDirection = EDodgeDirection::None;

	if (CurrentState == ECharacterState::Dodging)
	{
		OnDodgeEnded_StateHook();
	}
}

void ABaseCharacter::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("OnDodgeMontageEnded | Montage=%s | Interrupted=%d"),
		*GetNameSafe(Montage),
		bInterrupted ? 1 : 0);
	EndDodge();
}

void ABaseCharacter::FinishDeathSequence()
{
	if (bDeathSequenceFinished)
	{
		return;
	}

	bDeathSequenceFinished = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathFinishTimerHandle);
	}

	ActiveDeathMontage = nullptr;

	OnDeathFinished();

	OnCharacterDeathFinished.Broadcast(this);
}

void ABaseCharacter::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveDeathMontage.Get())
	{
		return;
	}

	FinishDeathSequence();
}
