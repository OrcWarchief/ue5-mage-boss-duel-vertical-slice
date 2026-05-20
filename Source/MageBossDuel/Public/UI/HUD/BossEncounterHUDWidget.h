// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossEncounterHUDWidget.generated.h"

class ABaseCharacter;
class UProgressBar;
class UTextBlock;
class UWidget;

UCLASS()
class MAGEBOSSDUEL_API UBossEncounterHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Boss HUD")
	void InitializeFromBoss(ABaseCharacter* InBossCharacter);

	UFUNCTION(BlueprintCallable, Category = "Boss HUD")
	void ClearBoss();

	UFUNCTION(BlueprintCallable, Category = "Boss HUD")
	void BeginHide();

	UFUNCTION(BlueprintPure, Category = "Boss HUD")
	ABaseCharacter* GetCachedBoss() const { return CachedBossCharacter; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> BossBarRoot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHealthLagBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss HUD|Lag", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float BossLagBarStartDelay = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss HUD|Lag", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BossLagBarInterpSpeed = 1.8f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss HUD")
	void OnBossHUDShown();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss HUD")
	void OnBossHUDHideRequested();

private:
	UPROPERTY()
	TObjectPtr<ABaseCharacter> CachedBossCharacter = nullptr;

	float BossLagDisplayedPercent = 1.f;
	float LastBossActualPercent = 1.f;
	float BossLagDelayRemaining = 0.f;

	void BindToBoss();
	void UnbindFromBoss();
	void RefreshFromBoss();

	void SetBossHUDVisible(bool bVisible);
	void UpdateBossHealthUI(float CurrentValue, float MaxValue, float Percent);
	void ResetBossLagBar(float CurrentHealthPercent);
	void UpdateBossLagBar(float CurrentHealthPercent, float DeltaTime);

	FText GetBossDisplayName() const;

	UFUNCTION()
	void HandleBossHealthChanged(float CurrentValue, float MaxValue, float Percent);
};
