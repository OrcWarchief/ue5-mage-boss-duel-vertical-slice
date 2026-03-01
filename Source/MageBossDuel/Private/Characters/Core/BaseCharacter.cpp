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

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// 컨트롤러 yaw를 캐릭터가 따라가게
	bUseControllerRotationYaw = true;
	// 이동 방향으로 자동 회전 끔
	GetCharacterMovement()->bOrientRotationToMovement = false; 
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

	// 전투/상태 초기화
	bIsAttacking = false;
	LastAttackTime = -9999.f;
	SetCharacterState(ECharacterState::Idle);

	// 이동속도 초기화
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = WalkSpeed;
	}
}

void ABaseCharacter::SetHealth(float NewHealth)
{
	if (CurrentState == ECharacterState::Dead) return;

	const float ClampedHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	CurrentHealth = ClampedHealth;
	
	if (CurrentHealth <= 0.f)
	{
		SetCharacterState(ECharacterState::Dead); // terminal 보장
	}
}

void ABaseCharacter::ApplyDamage(float DamageAmount, ABaseCharacter* DamageCauser) // Apply -> Recieve or Take Damage?
{
	if (!IsAlive())
	{
		return;
	}

	if (DamageAmount <= 0.f)
	{
		return;
	}

	// TODO : Defense/Poise 등 적용 
	const float FinalDamage = DamageAmount;

	const float NewHealth = CurrentHealth - FinalDamage;
	SetHealth(NewHealth); // 내부에서 Clamp + Die 처리

	if (IsAlive())
	{
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
	if (!CanBasicAttack())
	{
		return;
	}

	bIsAttacking = true;
	SetCharacterState(ECharacterState::Attacking);

	if (UWorld* World = GetWorld())
	{
		LastAttackTime = World->GetTimeSeconds();
	}

	// (BP에서 구현)애님 몽타주 재생 -> 애님노티파이에서 PerformBasicAttackHitCheck() -> EndBasicAttack() 호출
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && BasicAttackMontage)
	{
		AnimInstance->Montage_Play(BasicAttackMontage);
	}
	else
	{
		// 몽타주 없으면 즉시 발사 후 종료
		PerformBasicAttackHitCheck();
		EndBasicAttack();
	}
}

void ABaseCharacter::PerformBasicAttackHitCheck_Implementation()
{
	if (!IsAlive() || !bIsAttacking)
	{
		return;
	}

	// 마나 소모
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
	if (!bIsAttacking)
	{
		return;
	}
	bIsAttacking = false;

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
	const FVector& ViewLocation, 
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

	TArray<FOverlapResult> Candidates;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bAny = World->OverlapMultiByObjectType(
		Candidates,
		SearchCenter,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(MaxDistance),
		QueryParams
	);

	// 디버그 
	DrawDebugSphere(World, SearchCenter, MaxDistance, 24, FColor::Cyan, false, 1.0f, 0, 1.0f);

	if (!bAny) { return nullptr; }

	double BestScore = -DBL_MAX;
	AActor* BestTarget = nullptr;

	const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(MaxAngleDegrees));

	for (const FOverlapResult& Candidate : Candidates)
	{
		AActor* TargetCandidate = Candidate.GetActor();
		if (!IsValid(TargetCandidate) || TargetCandidate == this) { continue; }

		if (const ABaseCharacter* BaseChar = Cast<ABaseCharacter>(TargetCandidate))
		{
			if (!BaseChar->IsAlive()) { continue; }
		}

		const FVector CandidateAimLocation = GetTargetAimLocation(TargetCandidate);
		const FVector ToCandidate = CandidateAimLocation - ViewLocation;

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
				ViewLocation,
				CandidateAimLocation,
				VisibilityChannel,
				LoSParams
			);

			if (bHit && Hit.GetActor() != TargetCandidate)
			{
				continue;
			}
		}

		// 스코어: 정면에 가까울수록 +, 멀수록 -
		const double Score = (double)Dot * 1000.0 - (double)Dist;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = TargetCandidate;
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
		// 프로젝타일 초기 세팅?
	}
}

// ===== LifeCycle / Hit =====
void ABaseCharacter::OnDeathFinished()
{
	// 데스 몽타주 종료시점에서 호출하는 용도 2초 후 destroy
	SetLifeSpan(2.0f);
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

void ABaseCharacter::Die_Implementation()
{
	if (CurrentState != ECharacterState::Dead)
	{
		return;
	}
	
	// 상태 정리
	bIsAttacking = false;
	SetCharacterState(ECharacterState::Dead);

	// 이동 정지
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}

	// 콜리전 비활성화
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// PlayAnimMontage(DeathMontage);
	OnDeathFinished();
}

void ABaseCharacter::OnHitReaction_Implementation()
{
	if (bIsAttacking && CanBeInterrupted())
	{
		// 공격 강제 종료
		EndBasicAttack();
	}

	SetCharacterState(ECharacterState::Hit);
}

void ABaseCharacter::SetCharacterState(ECharacterState NewState)
{
	// 죽으면 다른 상태로 전이 불가능
	if (CurrentState == ECharacterState::Dead)
	{
		return;
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
	return true;
}