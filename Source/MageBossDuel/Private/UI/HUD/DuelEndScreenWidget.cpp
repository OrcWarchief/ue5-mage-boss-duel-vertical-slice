// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/DuelEndScreenWidget.h"

void UDuelEndScreenWidget::InitializeFromEncounterManager(ADuelEncounterManager* InEncounterManager, EDuelEndResult InEndResult)
{
	EncounterManager = InEncounterManager;
	EndResult = InEndResult;

	UE_LOG(LogTemp, Warning, TEXT("[RespawnUI] Initialized. Manager=%s Result=%d Widget=%s"),
		*GetNameSafe(EncounterManager),
		static_cast<int32>(EndResult),
		*GetNameSafe(this)
	);

	OnInitializedFromEncounterManager(EndResult);
}

bool UDuelEndScreenWidget::RequestRespawnFromDefeat()
{
	UE_LOG(LogTemp, Warning, TEXT("[RespawnUI] Widget RequestRespawnFromDefeat called. Manager=%s Result=%d"),
		*GetNameSafe(EncounterManager),
		static_cast<int32>(EndResult)
	);

	if (!IsValid(EncounterManager))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RespawnUI] Failed: EncounterManager is invalid"));
		return false;
	}

	const bool bResult = EncounterManager->RequestRespawnFromDefeat();

	UE_LOG(LogTemp, Warning, TEXT("[RespawnUI] Manager RequestRespawnFromDefeat returned %d"), bResult);

	return bResult;
}
