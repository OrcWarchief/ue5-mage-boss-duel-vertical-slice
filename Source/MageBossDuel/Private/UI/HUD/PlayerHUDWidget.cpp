// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/PlayerHUDWidget.h"

#include "Characters/Player/PlayerCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachePlayerCharacter();
	RefreshHUD();
}

void UPlayerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(CachedPlayerCharacter))
	{
		CachePlayerCharacter();
	}

	RefreshHUD();
}

void UPlayerHUDWidget::CachePlayerCharacter()
{
	CachedPlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
}

void UPlayerHUDWidget::RefreshHUD()
{
	if (!IsValid(CachedPlayerCharacter)) { return; }

	if (HealthBar)
	{
		HealthBar->SetPercent(CachedPlayerCharacter->GetHealthPercent());
	}

	if (ManaBar)
	{
		ManaBar->SetPercent(CachedPlayerCharacter->GetManaPercent());
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(
			FString::Printf(
				TEXT("HP %.0f / %.0f"),
				CachedPlayerCharacter->GetCurrentHealth(),
				CachedPlayerCharacter->GetMaxHealth()
			)
		));
	}

	if (ManaText)
	{
		ManaText->SetText(FText::FromString(
			FString::Printf(
				TEXT("MP %.0f / %.0f"),
				CachedPlayerCharacter->GetCurrentMana(),
				CachedPlayerCharacter->GetMaxMana()
			)
		));
	}
}
