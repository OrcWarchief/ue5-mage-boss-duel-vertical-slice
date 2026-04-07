// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/MBDPlayerController.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/TargetHUDWidget.h"
#include "Characters/Core/BaseCharacter.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

AMBDPlayerController::AMBDPlayerController()
{
    bShowMouseCursor = false;
}

void AMBDPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController())
    {
        ApplyMappingContexts();

        FInputModeGameOnly Mode;
        SetInputMode(Mode);

        if (PlayerHUDWidgetClass)
        {
            PlayerHUDWidget = CreateWidget<UPlayerHUDWidget>(this, PlayerHUDWidgetClass);
            if (PlayerHUDWidget)
            {
                PlayerHUDWidget->AddToViewport();

                if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetPawn()))
                {
                    PlayerHUDWidget->InitializeFromCharacter(BaseCharacter);
                }
            }
        }

        if (TargetHUDWidgetClass)
        {
            TargetHUDWidget = CreateWidget<UTargetHUDWidget>(this, TargetHUDWidgetClass);
            if (TargetHUDWidget)
            {
                TargetHUDWidget->AddToViewport(10);

                if (ABaseCharacter* PlayerCharacter = Cast<ABaseCharacter>(GetPawn()))
                {
                    TargetHUDWidget->InitializeFromPlayerCharacter(PlayerCharacter);
                }
            }
        }
    }
}

void AMBDPlayerController::ApplyMappingContexts()
{
    if (!GetLocalPlayer()) return;

    UEnhancedInputLocalPlayerSubsystem* Subsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (!Subsystem) return;

    if (IMC_Locomotion)
    {
        Subsystem->AddMappingContext(IMC_Locomotion, IMC_LocomotionPriority);
    }
    if (IMC_Combat)
    {
        Subsystem->AddMappingContext(IMC_Combat, IMC_CombatPriority);
    }
}