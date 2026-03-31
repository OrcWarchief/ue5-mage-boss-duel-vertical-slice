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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|NormalTarget")
	FVector2D NormalTargetBarScreenOffset = FVector2D(0.f, -6.f);

private:
	UPROPERTY()
	TObjectPtr<ABaseCharacter> CachedPlayerCharacter;

	void CachePlayerCharacter();
	void HideAllTargetUI();
	void UpdateTargetUI();

	bool ProjectWorldToWidget(const FVector& WorldLocation, FVector2D& OutWidgetPosition) const;
	void UpdateLockOnMarker(const FVector& WorldLocation);
	void UpdateNormalTargetBarPosition(ABaseCharacter* TargetCharacter);
	void UpdateHealthWidgets(ABaseCharacter* TargetCharacter);
};
