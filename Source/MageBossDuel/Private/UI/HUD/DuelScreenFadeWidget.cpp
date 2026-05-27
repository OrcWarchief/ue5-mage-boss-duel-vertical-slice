// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/DuelScreenFadeWidget.h"

void UDuelScreenFadeWidget::RequestFadeIn()
{
	OnFadeInRequested();
}

void UDuelScreenFadeWidget::RequestFadeOut()
{
	OnFadeOutRequested();
}