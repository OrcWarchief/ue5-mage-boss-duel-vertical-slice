// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/TargetHUDWidget.h"

#include "Characters/Core/BaseCharacter.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

void UTargetHUDWidget::InitializeFromPlayerCharacter(ABaseCharacter* InPlayerCharacter)
{
	CachedPlayerCharacter = InPlayerCharacter;
}

void UTargetHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachePlayerCharacter();
	HideAllTargetUI();
}

void UTargetHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateTargetUI();
}

void UTargetHUDWidget::CachePlayerCharacter()
{
	if (IsValid(CachedPlayerCharacter))
	{
		return;
	}

	CachedPlayerCharacter = Cast<ABaseCharacter>(GetOwningPlayerPawn());
}

void UTargetHUDWidget::HideAllTargetUI()
{
	if (LockOnMarker)
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (NormalTargetBarBox)
	{
		NormalTargetBarBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (BossBarBox)
	{
		BossBarBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTargetHUDWidget::UpdateTargetUI()
{
	if (!IsValid(CachedPlayerCharacter))
	{
		CachePlayerCharacter();
	}

	if (!IsValid(CachedPlayerCharacter))
	{
		HideAllTargetUI();
		return;
	}

	ABaseCharacter* TargetCharacter = Cast<ABaseCharacter>(CachedPlayerCharacter->GetLockOnTargetActor());
	if (!IsValid(TargetCharacter) || !TargetCharacter->IsAlive())
	{
		HideAllTargetUI();
		return;
	}

	UpdateLockOnMarker(TargetCharacter->GetLockOnWorldLocation());
	UpdateHealthWidgets(TargetCharacter);
}

void UTargetHUDWidget::UpdateLockOnMarker(const FVector& WorldLocation)
{
	if (!LockOnMarker)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FVector2D WidgetPosition;
	const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PC,
		WorldLocation,
		WidgetPosition,
		true
	);

	if (!bProjected)
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(LockOnMarker->Slot))
	{
		CanvasSlot->SetPosition(WidgetPosition);
	}

	LockOnMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTargetHUDWidget::UpdateHealthWidgets(ABaseCharacter* TargetCharacter)
{
	if (!IsValid(TargetCharacter))
	{
		HideAllTargetUI();
		return;
	}

	const float HealthPercent = TargetCharacter->GetHealthPercent();
	const bool bIsBossTarget = TargetCharacter->UsesBossTargetHUD();

	if (bIsBossTarget)
	{
		if (NormalTargetBarBox)
		{
			NormalTargetBarBox->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (BossBarBox)
		{
			BossBarBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}

		if (BossHealthBar)
		{
			BossHealthBar->SetPercent(HealthPercent);
		}

		if (BossNameText)
		{
			const FText DisplayName = TargetCharacter->GetTargetDisplayName().IsEmpty()
				? FText::FromString(TargetCharacter->GetName())
				: TargetCharacter->GetTargetDisplayName();

			BossNameText->SetText(DisplayName);
		}
	}
	else
	{
		if (BossBarBox)
		{
			BossBarBox->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (NormalTargetBarBox)
		{
			NormalTargetBarBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}

		if (NormalTargetHealthBar)
		{
			NormalTargetHealthBar->SetPercent(HealthPercent);
		}
	}
}