// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/DuelEndScreenWidget.h"

void UDuelEndScreenWidget::InitializeFromEncounterManager(ADuelEncounterManager* InEncounterManager, EDuelEndResult InEndResult)
{
	EncounterManager = InEncounterManager;
	EndResult = InEndResult;

	OnInitializedFromEncounterManager(EndResult);
}

bool UDuelEndScreenWidget::RequestRespawnFromDefeat()
{
	if (!IsValid(EncounterManager))
	{
		return false;
	}

	const bool bResult = EncounterManager->RequestRespawnFromDefeat();

	return bResult;
}
