// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/MBDRespawnSubsystem.h"

#include "Characters/Core/BaseCharacter.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UMBDRespawnSubsystem::SetActiveRestPoint(FName RestPointId, FName LevelName, const FTransform& RespawnTransform)
{
	ActiveRestPoint.bHasValidRestPoint = true;
	ActiveRestPoint.RestPointId = RestPointId;
	ActiveRestPoint.LevelName = LevelName;
	ActiveRestPoint.RespawnTransform = RespawnTransform;

	UE_LOG(LogTemp, Warning, TEXT("[RespawnSubsystem] SetActiveRestPoint Id=%s Level=%s Location=%s Rotation=%s"),
		*RestPointId.ToString(),
		*LevelName.ToString(),
		*RespawnTransform.GetLocation().ToString(),
		*RespawnTransform.Rotator().ToString()
	);
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

	UE_LOG(LogTemp, Warning, TEXT("[RespawnSubsystem] Level check. SavedLevel=%s CurrentLevel=%s"),
		*ActiveRestPoint.LevelName.ToString(),
		*CurrentLevelName.ToString()
	);

	if (!ActiveRestPoint.LevelName.IsNone() && ActiveRestPoint.LevelName != CurrentLevelName)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RespawnSubsystem] Failed: Level mismatch"));
		return false;
	}

	const FTransform& RespawnTransform = ActiveRestPoint.RespawnTransform;
	const FVector RespawnLocation = RespawnTransform.GetLocation();

	FRotator RespawnRotation = RespawnTransform.GetRotation().Rotator();
	RespawnRotation.Pitch = 0.0f; // Ensure the player doesn't spawn with an unintended pitch
	RespawnRotation.Roll = 0.0f;  // Ensure the player doesn't spawn with an unintended roll

	UE_LOG(LogTemp, Warning, TEXT("[RespawnSubsystem] Trying TeleportTo Location=%s Rotation=%s Pawn=%s"),
		*RespawnLocation.ToString(),
		*RespawnRotation.ToString(),
		*GetNameSafe(PlayerPawn)
	);

	if (UPawnMovementComponent* MovementComponent = PlayerPawn->GetMovementComponent())
	{
		MovementComponent->StopMovementImmediately();
	}

	const bool bTeleported = PlayerPawn->TeleportTo(
		RespawnLocation, 
		RespawnRotation, 
		false, 
		true
	);
	UE_LOG(LogTemp, Warning, TEXT("[RespawnSubsystem] TeleportTo returned %d"), bTeleported);

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

	return true;
}
