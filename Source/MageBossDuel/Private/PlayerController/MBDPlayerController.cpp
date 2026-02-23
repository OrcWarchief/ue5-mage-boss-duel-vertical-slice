// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/MBDPlayerController.h"
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