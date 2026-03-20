// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bLockOnActive)
	{
		UpdateLockOn(DeltaTime);
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!ensure(EIC)) return;

	if (ensure(IA_Move))
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &APlayerCharacter::OnMoveReleased);
		EIC->BindAction(IA_Move, ETriggerEvent::Canceled, this, &APlayerCharacter::OnMoveReleased);
	}
	if (ensure(IA_Look))	EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
	if (IA_Jump)			EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &APlayerCharacter::Jump);
	if (ensure(IA_LockOn))	EIC->BindAction(IA_LockOn, ETriggerEvent::Started, this, &APlayerCharacter::ToggleLockOn);
	if (ensure(IA_TargetSwitchX))
	{
		EIC->BindAction(IA_TargetSwitchX, ETriggerEvent::Triggered, this, &APlayerCharacter::OnTargetSwitchX);
		EIC->BindAction(IA_TargetSwitchX, ETriggerEvent::Completed, this, &APlayerCharacter::OnTargetSwitchXReleased);
		EIC->BindAction(IA_TargetSwitchX, ETriggerEvent::Canceled,  this, &APlayerCharacter::OnTargetSwitchXReleased);
	}
	if (ensure(IA_Dodge))	EIC->BindAction(IA_Dodge, ETriggerEvent::Started, this, &APlayerCharacter::Dodge);
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller) return;
	MovementVector = Value.Get<FVector2D>();
	if (IsDodging()) return;
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, MovementVector.Y);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void APlayerCharacter::OnMoveReleased(const FInputActionValue& Value)
{
	MovementVector = FVector2D::ZeroVector;
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void APlayerCharacter::ToggleLockOn(const FInputActionValue& Value)
{
	if (!IsLocallyControlled() || !Controller) return;
	if (IsDodging()) return;

	if (bLockOnActive && IsValid(LockOnTarget))
	{
		StopLockOn();
		return;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

	const FVector ViewForward = ViewRot.Vector();
	const FVector SearchCenter = GetActorLocation() + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f);
	
	AActor* Target = FindLockOnTarget(
		ViewLoc,
		ViewForward,
		SearchCenter,
		LockOnMaxDistance,
		LockOnMaxAngleDegrees,
		bLockOnRequireLineOfSight,
		LockOnVisibilityChannel
	);

	if (IsValid(Target))
	{
		StartLockOn(Target);
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("LockOn: No Target"));
	}
}

void APlayerCharacter::Jump()
{
	Super::Jump();
}

void APlayerCharacter::Dodge(const FInputActionValue& Value)
{
	TryStartDodge(MovementVector);
}

AActor* APlayerCharacter::GetLockOnTargetActor_Implementation() const
{
	return (bLockOnActive && IsValid(LockOnTarget)) ? LockOnTarget : nullptr;
}

void APlayerCharacter::StartLockOn(AActor* NewTarget)
{
	if (!IsValid(NewTarget) || NewTarget == this) return;

	bLockOnActive = true;
	LockOnTarget = NewTarget;

	// Lock On : character follow Controller Yaw;
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	if (GEngine)
	{
		const FString Msg = FString::Printf(TEXT("StartLockOn() : Lock On (On) : %s"), *NewTarget->GetName());
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, Msg);
	}
}

void APlayerCharacter::StopLockOn()
{
	bLockOnActive = false;
	LockOnTarget = nullptr;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Silver, TEXT("StopLockOn() : Lock On (Off)"));
	}
}

void APlayerCharacter::UpdateLockOn(float DeltaTime)
{
	if (IsDodging())
	{
		return;
	}

	if (!IsLocallyControlled() || !Controller)
	{
		StopLockOn();
		return;
	}

	if (!IsValid(LockOnTarget))
	{
		StopLockOn();
		return;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

	const FVector AimLoc = GetTargetAimLocation(LockOnTarget);
	const FVector To = AimLoc - ViewLoc;
	if (To.IsNearlyZero()) { return; }

	FRotator DesiredRot = To.Rotation();
	DesiredRot.Roll = 0.f;
	DesiredRot.Pitch = FMath::ClampAngle(DesiredRot.Pitch, -80.f, 80.f);

	const FRotator CurrentControlRot = Controller->GetControlRotation();
	const FRotator NextControlRot = FMath::RInterpTo(CurrentControlRot, DesiredRot, DeltaTime, LockOnInterpSpeed);
	Controller->SetControlRotation(NextControlRot);

	if (GEngine)
	{
		DrawDebugLine(GetWorld(), ViewLoc, AimLoc, FColor::Green, false, 0.f, 0, 2.f);
	}
}

void APlayerCharacter::OnTargetSwitchX(const FInputActionValue& Value)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("OnTargetSwitchX Start"));
	}
	if (!bLockOnActive || !IsValid(LockOnTarget)) return;
	if (!bTargetSwitchReady) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	const float Now = World->GetTimeSeconds();
	if ((Now - LastTargetSwitchTime) < TargetSwitchCooldown) return;

	const float Axis = Value.Get<float>();
	const int32 DirectionSign = (Axis > 0.f) ? 1 : -1;
	const bool bSwitched = TrySwitchLockOnTarget(DirectionSign);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.5f,
			bSwitched ? FColor::Green : FColor::Red,
			bSwitched ? TEXT("OnTargetSwitchX Switched") : TEXT("OnTargetSwitchX No Candidate")
		);
	}

	bTargetSwitchReady = false;
	LastTargetSwitchTime = Now;
}

void APlayerCharacter::OnTargetSwitchXReleased(const FInputActionValue& Value)
{
	bTargetSwitchReady = true;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Silver, TEXT("OnTargetSwitchXReleased"));
	}
}

bool APlayerCharacter::TrySwitchLockOnTarget(int32 DirectionSign)
{
	AActor* NewTarget = FindSwitchTarget(DirectionSign);
	if (!IsValid(NewTarget))
	{
		if (GEngine)
		{
			const TCHAR* Side = (DirectionSign > 0) ? TEXT("Right") : TEXT("Left");
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Silver, FString::Printf(TEXT("Switch %s: None"), Side));
		}
		return false;
	}
	LockOnTarget = NewTarget;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, FString::Printf(TEXT("Switch: %s"), *NewTarget->GetName()));
	}


	FVector ViewLoc; FRotator ViewRot;
	GetControllerViewPoint(ViewLoc, ViewRot);
	
	DrawDebugLine(GetWorld(), ViewLoc, GetTargetAimLocation(NewTarget), FColor::Magenta, false, 0.6f, 0, 2.f);

	return true;
}

AActor* APlayerCharacter::FindSwitchTarget(int32 DirectionSign) const
{
	UWorld* World = GetWorld();
	if (!World || !Controller || !IsValid(LockOnTarget)) return nullptr;

	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) return nullptr;

	int32 ViewW = 0, ViewH = 0;
	PC->GetViewportSize(ViewW, ViewH);
	if (ViewW <= 0 || ViewH <= 0) return nullptr;

	FVector ViewLoc;
	FRotator ViewRot;
	GetControllerViewPoint(ViewLoc, ViewRot);
	const FVector ViewForward = ViewRot.Vector();

	const FVector CurrAim = GetTargetAimLocation(LockOnTarget);
	FVector2D CurrScreen;
	if (!PC->ProjectWorldLocationToScreen(CurrAim, CurrScreen, false))
	{
		return nullptr;
	}
	const FVector2D CurrN(CurrScreen.X / (float)ViewW, CurrScreen.Y / (float)ViewH);

	TArray<FOverlapResult> Overlaps;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LockOnSwitchOverlap), false);
	QueryParams.AddIgnoredActor(this);

	const FVector SearchCenter = GetActorLocation();
	const float MaxDistance= LockOnMaxDistance;

	const bool bAny = World->OverlapMultiByObjectType(
		Overlaps,
		SearchCenter,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(MaxDistance),
		QueryParams
	);

	// 디버그 
	DrawDebugSphere(World, SearchCenter, MaxDistance, 12, FColor::Orange, false, 1.0f, 0, 1.0f);

	if (!bAny) { return nullptr; }

	float BestScore = FLT_MAX;
	AActor* BestTarget = nullptr;

	const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(TargetSwitchMaxAngleDegree));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 1.5f, FColor::Cyan,
			FString::Printf(TEXT("Overlap Count: %d"), Overlaps.Num())
		);
	}

	for (const FOverlapResult& R : Overlaps)
	{
		AActor* Candidate = R.GetActor();
		if (!IsValid(Candidate) || Candidate == this || Candidate == LockOnTarget) continue;

		if (const ABaseCharacter* BC = Cast<ABaseCharacter>(Candidate))
		{
			if (!BC->IsAlive()) continue;
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1, 1.0f, FColor::Yellow,
				FString::Printf(TEXT("Candidate Raw: %s"), *GetNameSafe(Candidate))
			);
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
		if (bLockOnRequireLineOfSight)
		{
			FHitResult Hit;
			FCollisionQueryParams LoSParams(SCENE_QUERY_STAT(LockOnSwitchLoS), true, this);

			const bool bHit = World->LineTraceSingleByChannel(
				Hit,
				ViewLoc,
				AimLoc,
				LockOnVisibilityChannel,
				LoSParams
			);

			if (bHit && Hit.GetActor() != Candidate)
			{
				continue;
			}
		}

		FVector2D Screen;
		if (!PC->ProjectWorldLocationToScreen(AimLoc, Screen, false)) continue;

		const FVector2D N(Screen.X / (float)ViewW, Screen.Y / (float)ViewH);

		if (N.X < 0.f || N.X > 1.f || N.Y < 0.f || N.Y > 1.f) continue;

		const float Dx = N.X - CurrN.X;
		const float Dy = FMath::Abs(N.Y - CurrN.Y);

		if (DirectionSign > 0) // 화면의 오른쪽이면
		{
			if (Dx <= TargetSwitchMinNormDx) continue;

			const float Score = Dx + Dy * TargetSwitchVerticalWeight + Dist * TargetSwitchDistanceWeight;
			if (Score < BestScore)
			{
				BestScore = Score;
				BestTarget = Candidate;
			}
		}
		else // DirectionSign < 0  화면의 왼쪽이면
		{
			if (-Dx <= TargetSwitchMinNormDx) continue;

			const float Score = (-Dx) + Dy * TargetSwitchVerticalWeight + Dist * TargetSwitchDistanceWeight;
			if (Score < BestScore)
			{
				BestScore = Score;
				BestTarget = Candidate;
			}
		}
	}

	return BestTarget;
}
