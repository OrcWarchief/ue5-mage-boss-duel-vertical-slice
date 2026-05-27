// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DuelScreenFadeWidget.generated.h"

/**
 * 
 */
UCLASS()
class MAGEBOSSDUEL_API UDuelScreenFadeWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Duel|Screen Fade")
	void RequestFadeIn();

	UFUNCTION(BlueprintCallable, Category = "Duel|Screen Fade")
	void RequestFadeOut();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Duel|Screen Fade")
	void OnFadeInRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Duel|Screen Fade")
	void OnFadeOutRequested();
};
