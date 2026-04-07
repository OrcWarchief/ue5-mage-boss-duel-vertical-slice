// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetHUDWidget.generated.h"

class ABaseCharacter;
class UImage;
class UProgressBar;
class USizeBox;
class UTextBlock;

UCLASS()
class MAGEBOSSDUEL_API UTargetHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void InitializeFromPlayerCharacter(ABaseCharacter* InPlayerCharacter);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LockOnMarker;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> NormalTargetBarBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> NormalTargetHealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> BossBarBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHealthLagBar;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Boss")
	float BossLagBarStartDelay = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Boss")
	float BossLagBarInterpSpeed = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|NormalTarget")
	FVector2D NormalTargetBarScreenOffset = FVector2D(0.f, -6.f);

private:
	UPROPERTY()
	TObjectPtr<ABaseCharacter> CachedPlayerCharacter;

	UPROPERTY()
	TObjectPtr<ABaseCharacter> CachedBossTarget;
	// ===== Boss Lag Bar =====
	float BossLagDisplayedPercent = 1.f;
	float LastBossActualPercent = 1.f;
	float BossLagDelayRemaining = 0.f;

	// ===== Target UI =====
	void CachePlayerCharacter();
	void HideAllTargetUI();
	void UpdateTargetUI();
	// ===== Widget Position =====
	bool ProjectWorldToWidget(const FVector& WorldLocation, FVector2D& OutWidgetPosition) const;
	void UpdateLockOnMarker(const FVector& WorldLocation);
	void UpdateNormalTargetBarPosition(ABaseCharacter* TargetCharacter);
	void UpdateHealthWidgets(ABaseCharacter* TargetCharacter);
	// ===== Boss Lag Bar =====
	void ResetBossLagBar(ABaseCharacter* BossCharacter, float CurrentHealthPercent);
	void UpdateBossLagBar(ABaseCharacter* BossCharacter, float CurrentHealthPercent);
};
