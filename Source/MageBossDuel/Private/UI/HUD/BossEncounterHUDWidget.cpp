// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/BossEncounterHUDWidget.h"

#include "Characters/Core/BaseCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UBossEncounterHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetBossHUDVisible(false);
}

void UBossEncounterHUDWidget::NativeDestruct()
{
	UnbindFromBoss();

	Super::NativeDestruct();
}

void UBossEncounterHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CachedBossCharacter && !IsValid(CachedBossCharacter))
	{
		ClearBoss();
		return;
	}

	if (!CachedBossCharacter)
	{
		return;
	}

	UpdateBossLagBar(CachedBossCharacter->GetHealthPercent(), InDeltaTime);
}

void UBossEncounterHUDWidget::InitializeFromBoss(ABaseCharacter* InBossCharacter)
{
	if (!IsValid(InBossCharacter))
	{
		ClearBoss();
		return;
	}

	if (CachedBossCharacter == InBossCharacter)
	{
		RefreshFromBoss();
		return;
	}

	UnbindFromBoss();

	CachedBossCharacter = InBossCharacter;

	BindToBoss();
	RefreshFromBoss();

	OnBossHUDShown();
}

void UBossEncounterHUDWidget::ClearBoss()
{
	UnbindFromBoss();

	CachedBossCharacter = nullptr;

	BossLagDisplayedPercent = 1.f;
	LastBossActualPercent = 1.f;
	BossLagDelayRemaining = 0.f;

	if (BossHealthBar)
	{
		BossHealthBar->SetPercent(1.f);
	}

	if (BossHealthLagBar)
	{
		BossHealthLagBar->SetPercent(1.f);
	}

	if (BossNameText)
	{
		BossNameText->SetText(FText::GetEmpty());
	}

	SetBossHUDVisible(false);
}

void UBossEncounterHUDWidget::BeginHide()
{
	UnbindFromBoss();

	CachedBossCharacter = nullptr;
	BossLagDelayRemaining = 0.f;

	OnBossHUDHideRequested();
}

void UBossEncounterHUDWidget::BindToBoss()
{
	if (!IsValid(CachedBossCharacter))
	{
		return;
	}

	CachedBossCharacter->OnHealthChanged.AddUniqueDynamic(
		this,
		&UBossEncounterHUDWidget::HandleBossHealthChanged
	);
}

void UBossEncounterHUDWidget::UnbindFromBoss()
{
	if (!IsValid(CachedBossCharacter))
	{
		return;
	}

	CachedBossCharacter->OnHealthChanged.RemoveDynamic(
		this,
		&UBossEncounterHUDWidget::HandleBossHealthChanged
	);
}

void UBossEncounterHUDWidget::RefreshFromBoss()
{
	if (!IsValid(CachedBossCharacter))
	{
		ClearBoss();
		return;
	}

	const float HealthPercent = CachedBossCharacter->GetHealthPercent();

	UpdateBossHealthUI(
		CachedBossCharacter->GetCurrentHealth(),
		CachedBossCharacter->GetMaxHealth(),
		HealthPercent
	);

	ResetBossLagBar(HealthPercent);

	SetBossHUDVisible(true);
}

void UBossEncounterHUDWidget::SetBossHUDVisible(bool bVisible)
{
	const ESlateVisibility NewVisibility = bVisible
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed;

	if (BossBarRoot)
	{
		BossBarRoot->SetVisibility(NewVisibility);
	}
	else
	{
		SetVisibility(NewVisibility);
	}
}

void UBossEncounterHUDWidget::UpdateBossHealthUI(float CurrentValue, float MaxValue, float Percent)
{
	(void)CurrentValue;
	(void)MaxValue;

	const float ClampedPercent = FMath::Clamp(Percent, 0.f, 1.f);

	if (BossHealthBar)
	{
		BossHealthBar->SetPercent(ClampedPercent);
	}

	if (BossNameText)
	{
		BossNameText->SetText(GetBossDisplayName());
	}
}

void UBossEncounterHUDWidget::ResetBossLagBar(float CurrentHealthPercent)
{
	const float ClampedPercent = FMath::Clamp(CurrentHealthPercent, 0.f, 1.f);

	BossLagDisplayedPercent = ClampedPercent;
	LastBossActualPercent = ClampedPercent;
	BossLagDelayRemaining = 0.f;

	if (BossHealthLagBar)
	{
		BossHealthLagBar->SetPercent(ClampedPercent);
	}
}

void UBossEncounterHUDWidget::UpdateBossLagBar(float CurrentHealthPercent, float DeltaTime)
{
	if (!BossHealthLagBar)
	{
		return;
	}

	const float ClampedPercent = FMath::Clamp(CurrentHealthPercent, 0.f, 1.f);

	const bool bJustTookDamage =
		ClampedPercent < LastBossActualPercent - KINDA_SMALL_NUMBER;

	const bool bJustHealed =
		ClampedPercent > LastBossActualPercent + KINDA_SMALL_NUMBER;

	if (bJustHealed)
	{
		BossLagDisplayedPercent = ClampedPercent;
		BossLagDelayRemaining = 0.f;
	}
	else
	{
		if (bJustTookDamage)
		{
			BossLagDelayRemaining = BossLagBarStartDelay;
		}

		if (BossLagDelayRemaining > 0.f)
		{
			BossLagDelayRemaining = FMath::Max(
				0.f,
				BossLagDelayRemaining - DeltaTime
			);
		}
		else if (BossLagDisplayedPercent > ClampedPercent)
		{
			BossLagDisplayedPercent = FMath::FInterpTo(
				BossLagDisplayedPercent,
				ClampedPercent,
				DeltaTime,
				BossLagBarInterpSpeed
			);

			if (FMath::IsNearlyEqual(BossLagDisplayedPercent, ClampedPercent, 0.001f))
			{
				BossLagDisplayedPercent = ClampedPercent;
			}
		}
	}

	LastBossActualPercent = ClampedPercent;
	BossHealthLagBar->SetPercent(BossLagDisplayedPercent);
}

FText UBossEncounterHUDWidget::GetBossDisplayName() const
{
	if (!IsValid(CachedBossCharacter))
	{
		return FText::GetEmpty();
	}

	const FText DisplayName = CachedBossCharacter->GetTargetDisplayName();
	if (!DisplayName.IsEmpty())
	{
		return DisplayName;
	}

	return FText::FromString(CachedBossCharacter->GetName());
}

void UBossEncounterHUDWidget::HandleBossHealthChanged(float CurrentValue, float MaxValue, float Percent)
{
	UpdateBossHealthUI(CurrentValue, MaxValue, Percent);
}