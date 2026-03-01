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
	// Super::Tick(DeltaTime);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!ensure(EIC)) return;

	if (ensure(IA_Move))	EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
	if (ensure(IA_Look))	EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
	if (IA_Jump)			EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &APlayerCharacter::Jump);
	if (ensure(IA_LockOn))	EIC->BindAction(IA_LockOn, ETriggerEvent::Started, this, &APlayerCharacter::ToggleLockOn);
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller) return;
	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, MovementVector.Y);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, MovementVector.X);
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

	if (bLockOnActive && IsValid(LockOnTarget))
	{
		StopLockOn();
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector ViewForward = ViewRotation.Vector();
	const FVector SearchCenter = GetActorLocation() + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f);
	
	AActor* Target = FindLockOnTarget(
		ViewLocation,
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

AActor* APlayerCharacter::GetLockOnTargetActor_Implementation() const
{
	return (bLockOnActive && IsValid(LockOnTarget)) ? LockOnTarget : nullptr;
}

void APlayerCharacter::StartLockOn(AActor* NewTarget)
{
	if (!IsValid(NewTarget) || NewTarget == this) return;

	bLockOnActive = true;
	LockOnTarget = NewTarget;

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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Silver, TEXT("StopLockOn() : Lock On (Off)"));
	}
}
