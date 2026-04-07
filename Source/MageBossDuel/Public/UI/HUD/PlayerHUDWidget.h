// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class ABaseCharacter;

UCLASS()
class MAGEBOSSDUEL_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void InitializeFromCharacter(ABaseCharacter* InCharacter);
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ManaBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ManaText;

private:
	UPROPERTY()
	TObjectPtr<ABaseCharacter> CachedCharacter;

	void BindToCharacter();
	void UnbindFromCharacter();
	void RefreshAllFromCharacter();
	
	void UpdateHealthUI(float CurrentValue, float MaxValue, float Percent);
	void UpdateManaUI(float CurrentValue, float MaxValue, float Percent);

	UFUNCTION()
	void HandleHealthChanged(float CurrentValue, float MaxValue, float Percent);

	UFUNCTION()
	void HandleManaChanged(float CurrentValue, float MaxValue, float Percent);
};
