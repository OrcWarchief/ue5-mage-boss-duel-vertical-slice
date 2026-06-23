// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/BossAnimInstance.h"
#include "Characters/Boss/MageBossCharacter.h"

#include "GameFramework/Pawn.h"

void UBossAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	APawn* PawnOwner = TryGetPawnOwner();
	
	if (IsValid(PawnOwner))
	{
		BossOwner = Cast<AMageBossCharacter>(PawnOwner);
	}
}

void UBossAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!IsValid(BossOwner))
	{
		return;
	}
	
	if (!BossOwner)
	{
		APawn* PawnOwner = TryGetPawnOwner();
		if (IsValid(PawnOwner))
		{
			BossOwner = Cast<AMageBossCharacter>(PawnOwner);
		}
	}

	if (!BossOwner)
	{
		Speed2D = 0.0f;
		LocalForwardSpeed = 0.0f;
		LocalRightSpeed = 0.0f;
		bShouldMove = false;
		bMoveLeft = false;
		bMoveRight = false;
		CurrentMoveSide = EBossMoveSide::None;
		return;
	}

	const FVector Velocity = BossOwner->GetVelocity();
	Speed2D = Velocity.Size2D();

	const FTransform ActorTransform = BossOwner->GetActorTransform();
	const FVector LocalVelocity = ActorTransform.InverseTransformVectorNoScale(Velocity);

	LocalForwardSpeed = LocalVelocity.X;
	LocalRightSpeed = LocalVelocity.Y;

	bShouldMove = Speed2D > MoveSpeedThreshold;

	bMoveLeft = LocalRightSpeed < -SideSpeedThreshold;
	bMoveRight = LocalRightSpeed > SideSpeedThreshold;

	if (bMoveLeft)
	{
		CurrentMoveSide = EBossMoveSide::Left;
	}
	else if (bMoveRight)
	{
		CurrentMoveSide = EBossMoveSide::Right;
	}
	else
	{
		CurrentMoveSide = EBossMoveSide::None;
	}
	if (CurrentMoveSide != EBossMoveSide::None)
	{
		LastMoveSide = CurrentMoveSide;
	}
}
