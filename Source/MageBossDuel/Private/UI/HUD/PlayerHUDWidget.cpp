// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/PlayerHUDWidget.h"

#include "Characters/Core/BaseCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerHUDWidget::InitializeFromCharacter(ABaseCharacter* InCharacter)
{
	if (CachedCharacter == InCharacter)
	{
		RefreshAllFromCharacter();
		return;
	}

	UnbindFromCharacter();
	CachedCharacter = InCharacter;
	BindToCharacter();
	RefreshAllFromCharacter();
}

void UPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeFromCharacter(Cast<ABaseCharacter>(GetOwningPlayerPawn()));
}

void UPlayerHUDWidget::NativeDestruct()
{
	UnbindFromCharacter();

	Super::NativeDestruct();
}

void UPlayerHUDWidget::BindToCharacter()
{
	if (!IsValid(CachedCharacter)) { return; }

	CachedCharacter->OnHealthChanged.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleHealthChanged);
	CachedCharacter->OnManaChanged.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleManaChanged);
}

void UPlayerHUDWidget::UnbindFromCharacter()
{
	if (!IsValid(CachedCharacter)) { return; }

	CachedCharacter->OnHealthChanged.RemoveDynamic(this, &UPlayerHUDWidget::HandleHealthChanged);
	CachedCharacter->OnManaChanged.RemoveDynamic(this, &UPlayerHUDWidget::HandleManaChanged);
}

void UPlayerHUDWidget::RefreshAllFromCharacter()
{
	if (!IsValid(CachedCharacter)) { return; }

	UpdateHealthUI(
		CachedCharacter->GetCurrentHealth(),
		CachedCharacter->GetMaxHealth(),
		CachedCharacter->GetHealthPercent()
	);

	UpdateManaUI(
		CachedCharacter->GetCurrentMana(),
		CachedCharacter->GetMaxMana(),
		CachedCharacter->GetManaPercent()
	);
}

void UPlayerHUDWidget::UpdateHealthUI(float CurrentValue, float MaxValue, float Percent)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(Percent);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(
			FString::Printf(TEXT("HP %.0f / %.0f"), CurrentValue, MaxValue)
		));
	}
}

void UPlayerHUDWidget::UpdateManaUI(float CurrentValue, float MaxValue, float Percent)
{
	if (ManaBar)
	{
		ManaBar->SetPercent(Percent);
	}

	if (ManaText)
	{
		ManaText->SetText(FText::FromString(
			FString::Printf(TEXT("MP %.0f / %.0f"), CurrentValue, MaxValue)
		));
	}
}

void UPlayerHUDWidget::HandleHealthChanged(float CurrentValue, float MaxValue, float Percent)
{
	UpdateHealthUI(CurrentValue, MaxValue, Percent);
}

void UPlayerHUDWidget::HandleManaChanged(float CurrentValue, float MaxValue, float Percent)
{
	UpdateManaUI(CurrentValue, MaxValue, Percent);
}