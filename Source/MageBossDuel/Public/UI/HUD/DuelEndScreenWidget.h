// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GamePlay/DuelEncounterManager.h"
#include "DuelEndScreenWidget.generated.h"

/**
 * 
 */
UCLASS()
class MAGEBOSSDUEL_API UDuelEndScreenWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Duel|End Screen")
	void InitializeFromEncounterManager(
		ADuelEncounterManager* InEncounterManager,
		EDuelEndResult InEndResult
	);

	UFUNCTION(BlueprintCallable, Category = "Duel|End Screen")
	bool RequestRespawnFromDefeat();

	UFUNCTION(BlueprintPure, Category = "Duel|End Screen")
	ADuelEncounterManager* GetEncounterManager() const { return EncounterManager; }

	UFUNCTION(BlueprintPure, Category = "Duel|End Screen")
	EDuelEndResult GetEndResult() const { return EndResult; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Duel|End Screen")
	void OnInitializedFromEncounterManager(EDuelEndResult InEndResult);

private:
	UPROPERTY()
	TObjectPtr<ADuelEncounterManager> EncounterManager = nullptr;

	UPROPERTY()
	EDuelEndResult EndResult = EDuelEndResult::None;
};
