#include "Characters/Boss/MageBossCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

AMageBossCharacter::AMageBossCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->VisibilityBasedAnimTickOption =
			EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}

// ===== Target =====

void AMageBossCharacter::SetCombatTarget(AActor* NewTarget)
{
	CurrentCombatTarget = (IsValid(NewTarget) && NewTarget != this) ? NewTarget : nullptr;
}

bool AMageBossCharacter::IsLockOnActive() const
{
	return IsValid(CurrentCombatTarget.Get());
}

AActor* AMageBossCharacter::GetCurrentLockOnTarget() const
{
	return CurrentCombatTarget.Get();
}

AActor* AMageBossCharacter::GetLockOnTargetActor_Implementation() const
{
	return CurrentCombatTarget.Get();
}

// ===== Teleport Public =====

bool AMageBossCharacter::CanStartTeleport(EDodgeDirection RequestedDirection) const
{
	if (!IsAlive())
	{
		return false;
	}

	const ECharacterState State = GetCurrentState();
	if (State != ECharacterState::Idle && State != ECharacterState::Moving)
	{
		return false;
	}

	if (TeleportRuntime.Phase != ETeleportPhase::None)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if ((World->GetTimeSeconds() - LastTeleportTime) < TeleportCooldown)
	{
		return false;
	}

	const USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !MeshComp->GetAnimInstance())
	{
		return false;
	}

	const EDodgeDirection Direction =
		RequestedDirection == EDodgeDirection::None
		? ChooseTeleportDirectionForAI()
		: RequestedDirection;

	if (Direction == EDodgeDirection::None)
	{
		return false;
	}

	if (!HasAnyValidTeleportMontagePair())
	{
		return false;
	}

	return true;
}

bool AMageBossCharacter::TryStartTeleport(EDodgeDirection RequestedDirection)
{
	const EDodgeDirection InitialDirection =
		RequestedDirection == EDodgeDirection::None
		? ChooseTeleportDirectionForAI()
		: RequestedDirection;

	if (!CanStartTeleport(InitialDirection))
	{
		return false;
	}

	FVector Destination = FVector::ZeroVector;
	EDodgeDirection ResolvedDirection = EDodgeDirection::None;

	if (!FindTeleportDestination(InitialDirection, Destination, ResolvedDirection))
	{
		return false;
	}

	const int32 MontagePairIndex = ChooseTeleportMontagePairIndex();
	if (MontagePairIndex == INDEX_NONE)
	{
		return false;
	}

	UAnimMontage* BeginMontage = GetTeleportBeginMontage(MontagePairIndex);
	if (!BeginMontage)
	{
		return false;
	}

	BeginTeleport(BeginMontage, ResolvedDirection, Destination, MontagePairIndex);
	return true;
}

void AMageBossCharacter::ExecuteTeleport()
{
	if (TeleportRuntime.Phase != ETeleportPhase::Startup)
	{
		return;
	}

	if (!IsAlive())
	{
		CancelTeleport(false, false);
		return;
	}

	TeleportRuntime.Phase = ETeleportPhase::Hidden;
	SetTeleportHiddenState(true);

	const FRotator NewRotation = bFaceTargetOnReappear
		? MakeFacingRotationAtLocation(TeleportRuntime.Destination)
		: GetActorRotation();

	// 목적지 검사는 FindTeleportDestination에서 끝남, bNoCheck=true.
	// 실패 시 숨김/충돌/이동 상태를 즉시 복구
	const bool bTeleported = TeleportTo(
		TeleportRuntime.Destination,
		NewRotation,
		false,
		true
	);

	if (!bTeleported)
	{
		CancelTeleport(true, true);
	}
}

void AMageBossCharacter::ReappearTeleport()
{
	if (TeleportRuntime.Phase != ETeleportPhase::Hidden)
	{
		return;
	}

	TeleportRuntime.Phase = ETeleportPhase::Recovery;
	SetTeleportHiddenState(false);

	if (bFaceTargetOnReappear)
	{
		SetActorRotation(MakeFacingRotationAtLocation(GetActorLocation()));
	}
}

// ===== BaseCharacter Hooks =====

void AMageBossCharacter::OnHitReaction_Implementation()
{
	if (IsTeleporting())
	{
		// 피격 리액션은 Super가 처리하므로 여기서는 텔포 런타임만 복구
		CancelTeleport(false, true);
	}

	Super::OnHitReaction_Implementation();
}

void AMageBossCharacter::Die_Implementation()
{
	if (IsTeleporting())
	{
		// 사망 처리는 Super가 이동/충돌 비활성화를 다시 처리
		CancelTeleport(false, false);
	}

	Super::Die_Implementation();
}

// ===== Teleport Flow =====

void AMageBossCharacter::BeginTeleport(
	UAnimMontage* BeginMontage,
	EDodgeDirection Direction,
	const FVector& Destination,
	int32 MontagePairIndex
)
{
	if (!BeginMontage)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	TeleportRuntime.Direction = Direction;
	TeleportRuntime.Phase = ETeleportPhase::Startup;
	TeleportRuntime.Destination = Destination;
	TeleportRuntime.MontagePairIndex = MontagePairIndex;
	TeleportRuntime.bMeshHidden = false;

	ActiveTeleportMontage = BeginMontage;
	ActiveTeleportMontageStage = ETeleportMontageStage::Begin;

	SetCharacterState(ECharacterState::Attacking);
	FreezeMovementForTeleport();

	if (bFaceTargetOnTeleportStart)
	{
		FaceWorldDirection(GetLockOnBasisForward());
	}

	const float PlayLength = AnimInstance->Montage_Play(BeginMontage);
	if (PlayLength <= 0.0f)
	{
		CancelTeleport(true, true);
		return;
	}

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &AMageBossCharacter::OnTeleportBeginMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, BeginMontage);

	if (UWorld* World = GetWorld())
	{
		LastTeleportTime = World->GetTimeSeconds();
	}

	LastTeleportDirection = Direction;
	LastTeleportMontagePairIndex = MontagePairIndex;
}

void AMageBossCharacter::EndTeleport()
{
	if (!IsTeleporting())
	{
		return;
	}

	SetTeleportHiddenState(false);

	TeleportRuntime = FTeleportRuntime();
	ActiveTeleportMontage = nullptr;
	ActiveTeleportMontageStage = ETeleportMontageStage::None;

	RestoreMovementAfterTeleport();

	if (IsAlive())
	{
		SetCharacterState(
			GetVelocity().Size2D() > 10.0f
			? ECharacterState::Moving
			: ECharacterState::Idle
		);
	}
}

void AMageBossCharacter::CancelTeleport(bool bRestoreNeutralState, bool bRestoreMovement)
{
	const bool bHadTeleportState =
		TeleportRuntime.Phase != ETeleportPhase::None ||
		TeleportRuntime.bMeshHidden ||
		ActiveTeleportMontage != nullptr;

	if (!bHadTeleportState)
	{
		return;
	}

	UAnimMontage* MontageToStop = ActiveTeleportMontage.Get();

	SetTeleportHiddenState(false);

	TeleportRuntime = FTeleportRuntime();
	ActiveTeleportMontage = nullptr;
	ActiveTeleportMontageStage = ETeleportMontageStage::None;

	if (bRestoreMovement)
	{
		RestoreMovementAfterTeleport();
	}

	if (bRestoreNeutralState && IsAlive())
	{
		SetCharacterState(
			GetVelocity().Size2D() > 10.0f
			? ECharacterState::Moving
			: ECharacterState::Idle
		);
	}

	if (MontageToStop)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			if (AnimInstance->Montage_IsPlaying(MontageToStop))
			{
				AnimInstance->Montage_Stop(0.05f, MontageToStop);
			}
		}
	}
}

void AMageBossCharacter::PlayTeleportEndMontage()
{
	if (TeleportRuntime.Phase != ETeleportPhase::Hidden)
	{
		CancelTeleport(true, true);
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		ReappearTeleport();
		EndTeleport();
		return;
	}

	UAnimMontage* EndMontage = GetTeleportEndMontage(TeleportRuntime.MontagePairIndex);
	if (!EndMontage)
	{
		ReappearTeleport();
		EndTeleport();
		return;
	}

	ActiveTeleportMontage = EndMontage;
	ActiveTeleportMontageStage = ETeleportMontageStage::End;

	const float PlayLength = AnimInstance->Montage_Play(EndMontage);
	if (PlayLength <= 0.0f)
	{
		ReappearTeleport();
		EndTeleport();
		return;
	}

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(
		this,
		&AMageBossCharacter::OnTeleportEndMontageEnded
	);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, EndMontage);
}

// ===== Teleport Selection =====

EDodgeDirection AMageBossCharacter::ChooseTeleportDirectionForAI() const
{
	const AActor* Target = CurrentCombatTarget.Get();

	if (!IsValid(Target))
	{
		return EDodgeDirection::Backward;
	}

	const float DistanceToTarget = FVector::Dist2D(GetActorLocation(), Target->GetActorLocation());

	TArray<EDodgeDirection> Options;

	if (DistanceToTarget <= TeleportNearDistance)
	{
		Options = {
			EDodgeDirection::Backward,
			EDodgeDirection::BackwardLeft,
			EDodgeDirection::BackwardRight
		};
	}
	else if (DistanceToTarget >= TeleportFarDistance)
	{
		Options = {
			EDodgeDirection::ForwardLeft,
			EDodgeDirection::ForwardRight,
			EDodgeDirection::Forward
		};
	}
	else
	{
		Options = {
			EDodgeDirection::Left,
			EDodgeDirection::Right,
			EDodgeDirection::BackwardLeft,
			EDodgeDirection::BackwardRight
		};
	}

	if (Options.Num() == 0)
	{
		return EDodgeDirection::Backward;
	}

	const int32 PickedIndex = FMath::RandRange(0, Options.Num() - 1);
	EDodgeDirection PickedDirection = Options[PickedIndex];

	if (Options.Num() > 1 && PickedDirection == LastTeleportDirection)	// TODO: Change to weight penalty
	{
		PickedDirection = Options[(PickedIndex + 1) % Options.Num()];
	}

	return PickedDirection;
}

bool AMageBossCharacter::HasAnyValidTeleportMontagePair() const
{
	for (const FTeleportMontagePair& Pair : TeleportMontagePairs)
	{
		if (Pair.IsValidPair())
		{
			return true;
		}
	}

	return false;
}

int32 AMageBossCharacter::ChooseTeleportMontagePairIndex() const
{
	TArray<int32> ValidIndices;

	for (int32 Index = 0; Index < TeleportMontagePairs.Num(); ++Index)
	{
		if (TeleportMontagePairs[Index].IsValidPair())
		{
			ValidIndices.Add(Index);
		}
	}

	if (ValidIndices.Num() == 0)
	{
		return INDEX_NONE;
	}

	int32 PickedArrayIndex = FMath::RandRange(0, ValidIndices.Num() - 1);

	if (ValidIndices.Num() > 1 && ValidIndices[PickedArrayIndex] == LastTeleportMontagePairIndex)
	{
		PickedArrayIndex = (PickedArrayIndex + 1) % ValidIndices.Num();
	}

	return ValidIndices[PickedArrayIndex];
}

UAnimMontage* AMageBossCharacter::GetTeleportBeginMontage(int32 PairIndex) const
{
	if (!TeleportMontagePairs.IsValidIndex(PairIndex))
	{
		return nullptr;
	}

	return TeleportMontagePairs[PairIndex].BeginMontage.Get();
}

UAnimMontage* AMageBossCharacter::GetTeleportEndMontage(int32 PairIndex) const
{
	if (!TeleportMontagePairs.IsValidIndex(PairIndex))
	{
		return nullptr;
	}

	return TeleportMontagePairs[PairIndex].EndMontage.Get();
}

void AMageBossCharacter::OnTeleportBeginMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (ActiveTeleportMontageStage != ETeleportMontageStage::Begin)
	{
		return;
	}

	if (Montage != ActiveTeleportMontage.Get())
	{
		return;
	}

	ActiveTeleportMontage = nullptr;
	ActiveTeleportMontageStage = ETeleportMontageStage::None;

	if (bInterrupted)
	{
		CancelTeleport(true, true);
		return;
	}

	if (TeleportRuntime.Phase == ETeleportPhase::Startup)
	{
		CancelTeleport(true, true);
		return;
	}

	if (TeleportRuntime.Phase == ETeleportPhase::Hidden)
	{
		PlayTeleportEndMontage();
		return;
	}

	if (TeleportRuntime.Phase == ETeleportPhase::Recovery)
	{
		EndTeleport();
	}
}

void AMageBossCharacter::OnTeleportEndMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (ActiveTeleportMontageStage != ETeleportMontageStage::End)
	{
		return;
	}

	if (Montage != ActiveTeleportMontage.Get())
	{
		return;
	}

	EndTeleport();
}

// ===== Destination =====

bool AMageBossCharacter::FindTeleportDestination(
	EDodgeDirection RequestedDirection,
	FVector& OutLocation,
	EDodgeDirection& OutResolvedDirection
) const
{
	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();

	if (!World || !Capsule)
	{
		return false;
	}

	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	TArray<EDodgeDirection> DirectionOrder;
	DirectionOrder.Reserve(9);

	DirectionOrder.Add(RequestedDirection);
	DirectionOrder.Add(EDodgeDirection::BackwardLeft);
	DirectionOrder.Add(EDodgeDirection::BackwardRight);
	DirectionOrder.Add(EDodgeDirection::Left);
	DirectionOrder.Add(EDodgeDirection::Right);
	DirectionOrder.Add(EDodgeDirection::Backward);
	DirectionOrder.Add(EDodgeDirection::ForwardLeft);
	DirectionOrder.Add(EDodgeDirection::ForwardRight);
	DirectionOrder.Add(EDodgeDirection::Forward);

	const TArray<float> DistanceScales = { 1.0f, 0.85f, 0.7f, 0.55f };

	for (const EDodgeDirection CandidateDirection : DirectionOrder)
	{
		if (CandidateDirection == EDodgeDirection::None)
		{
			continue;
		}

		const FVector DirectionWorld = TeleportDirectionToWorld(CandidateDirection);
		if (DirectionWorld.IsNearlyZero())
		{
			continue;
		}

		for (const float DistanceScale : DistanceScales)
		{
			const float CandidateDistance = FMath::Max(
				TeleportMinDistance,
				TeleportDistance * DistanceScale
			);

			const FVector RawCandidate =
				GetActorLocation() + DirectionWorld * CandidateDistance;

			const FVector TraceStart =
				RawCandidate + FVector::UpVector * TeleportGroundTraceUp;

			const FVector TraceEnd =
				RawCandidate - FVector::UpVector * TeleportGroundTraceDown;

			FHitResult GroundHit;
			FCollisionQueryParams GroundQueryParams(
				FName(TEXT("BossTeleportGroundTrace")),
				false,
				this
			);

			const bool bGroundHit = World->LineTraceSingleByChannel(
				GroundHit,
				TraceStart,
				TraceEnd,
				TeleportGroundTraceChannel.GetValue(),
				GroundQueryParams
			);

			if (!bGroundHit)
			{
				continue;
			}

			if (GroundHit.ImpactNormal.Z < TeleportMinGroundNormalZ)
			{
				continue;
			}

			const FVector CapsuleLocation =
				GroundHit.ImpactPoint +
				FVector(0.0f, 0.0f, CapsuleHalfHeight + TeleportGroundOffset);

			const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
				FMath::Max(1.0f, CapsuleRadius - 2.0f),
				FMath::Max(1.0f, CapsuleHalfHeight - 2.0f)
			);

			FCollisionQueryParams OverlapQueryParams(
				FName(TEXT("BossTeleportOverlap")),
				false,
				this
			);

			const bool bBlocked = World->OverlapBlockingTestByChannel(
				CapsuleLocation,
				FQuat::Identity,
				TeleportOverlapChannel.GetValue(),
				CapsuleShape,
				OverlapQueryParams
			);

			if (bBlocked)
			{
				continue;
			}

			OutLocation = CapsuleLocation;
			OutResolvedDirection = CandidateDirection;
			return true;
		}
	}

	return false;
}

FVector AMageBossCharacter::TeleportDirectionToWorld(EDodgeDirection Direction) const
{
	const FVector Forward = GetLockOnBasisForward();
	const FVector Right = GetLockOnBasisRight();

	switch (Direction)
	{
	case EDodgeDirection::Forward:
		return Forward;

	case EDodgeDirection::Backward:
		return -Forward;

	case EDodgeDirection::Left:
		return -Right;

	case EDodgeDirection::Right:
		return Right;

	case EDodgeDirection::ForwardLeft:
		return (Forward - Right).GetSafeNormal();

	case EDodgeDirection::ForwardRight:
		return (Forward + Right).GetSafeNormal();

	case EDodgeDirection::BackwardLeft:
		return (-Forward - Right).GetSafeNormal();

	case EDodgeDirection::BackwardRight:
		return (-Forward + Right).GetSafeNormal();

	default:
		return FVector::ZeroVector;
	}
}

FRotator AMageBossCharacter::MakeFacingRotationAtLocation(const FVector& WorldLocation) const
{
	const AActor* Target = CurrentCombatTarget.Get();

	if (IsValid(Target))
	{
		FVector ToTarget = Target->GetActorLocation() - WorldLocation;
		ToTarget.Z = 0.0f;

		if (!ToTarget.IsNearlyZero())
		{
			return ToTarget.Rotation();
		}
	}

	return GetActorRotation();
}

// ===== Visibility / Movement =====

void AMageBossCharacter::SetTeleportHiddenState(bool bShouldHide)
{
	TeleportRuntime.bMeshHidden = bShouldHide;

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetVisibility(!bShouldHide, true);
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(
			bShouldHide
			? ECollisionEnabled::NoCollision
			: ECollisionEnabled::QueryAndPhysics
		);
	}
}

void AMageBossCharacter::FreezeMovementForTeleport()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
}

void AMageBossCharacter::RestoreMovementAfterTeleport()
{
	if (!IsAlive())
	{
		return;
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}
}