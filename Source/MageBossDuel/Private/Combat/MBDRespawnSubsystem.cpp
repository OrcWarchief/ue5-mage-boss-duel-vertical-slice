// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/MBDRespawnSubsystem.h"

#include "Characters/Core/BaseCharacter.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UMBDRespawnSubsystem::SetActiveRestPoint(FName RestPointId, FName LevelName, const FTransform& RespawnTransform)
{
	ActiveRestPoint.bHasValidRestPoint = true;
	ActiveRestPoint.RestPointId = RestPointId;
	ActiveRestPoint.LevelName = LevelName;
	ActiveRestPoint.RespawnTransform = RespawnTransform;
}

void UMBDRespawnSubsystem::ClearActiveRestPoint()
{
	ActiveRestPoint = FRespawnPointData();
}

bool UMBDRespawnSubsystem::RespawnPlayerAtActiveRestPoint(APlayerController* PlayerController)
{
	if (!ActiveRestPoint.bHasValidRestPoint)
	{
		return false;
	}

	if (!IsValid(PlayerController))
	{
		return false;
	}

	APawn* PlayerPawn = PlayerController->GetPawn();
	if (!IsValid(PlayerPawn))
	{
		return false;
	}

	const FName CurrentLevelName = FName(*UGameplayStatics::GetCurrentLevelName(PlayerController, true));

	if (!ActiveRestPoint.LevelName.IsNone() && ActiveRestPoint.LevelName != CurrentLevelName)
	{
		return false;
	}

	const FTransform& RespawnTransform = ActiveRestPoint.RespawnTransform;
	FVector RespawnLocation = RespawnTransform.GetLocation();

	FRotator RespawnRotation = RespawnTransform.GetRotation().Rotator();
	RespawnRotation.Pitch = 0.0f; // Ensure the player doesn't spawn with an unintended pitch
	RespawnRotation.Roll = 0.0f;  // Ensure the player doesn't spawn with an unintended roll

	if (UPawnMovementComponent* MovementComponent = PlayerPawn->GetMovementComponent())
	{
		MovementComponent->StopMovementImmediately();
	}

	UWorld* World = PlayerPawn->GetWorld();
	if (!World)
	{
		return false;
	}

	// 저장된 위치 주변에서 캐릭터가 들어갈 수 있는 안전한 위치를 찾는다.
	if (!World->FindTeleportSpot(
		PlayerPawn,
		RespawnLocation,
		RespawnRotation
	))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Failed to find a safe respawn spot near %s"),
			*RespawnTransform.GetLocation().ToString()
		);

		return false;
	}

	const bool bTeleported = PlayerPawn->TeleportTo(
		RespawnLocation, 
		RespawnRotation, 
		false, 
		true
	);

	if (!bTeleported)
	{
		return false;
	}

	PlayerController->SetControlRotation(RespawnRotation);

	if (UPawnMovementComponent* MovementComponent = PlayerPawn->GetMovementComponent())
	{
		MovementComponent->StopMovementImmediately();
	}

	if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(PlayerPawn))
	{
		BaseCharacter->ReviveForRespawn();
	}

	// 리스폰 후 PlayerController 입력 상태 복구
	PlayerController->ResetIgnoreMoveInput();
	PlayerController->ResetIgnoreLookInput();

	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = false;

	return true;
}

bool UMBDRespawnSubsystem::BeginReloadRespawnAtActiveRestPoint(const UObject* WorldContextObject)
{
	if (!ActiveRestPoint.bHasValidRestPoint)
	{
		return false;
	}

	if (!IsValid(WorldContextObject))
	{
		return false;
	}

	if (ActiveRestPoint.LevelName.IsNone())
	{
		return false;
	}

	bPendingRespawnAfterLevelLoad = true;

	UGameplayStatics::OpenLevel(
		WorldContextObject,
		ActiveRestPoint.LevelName
	);

	return true;
}

void UMBDRespawnSubsystem::ConsumePendingRespawnAfterLevelLoad()
{
	bPendingRespawnAfterLevelLoad = false;
}
