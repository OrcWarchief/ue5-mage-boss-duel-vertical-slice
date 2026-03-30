// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MBDPlayerController.generated.h"

class UInputMappingContext;
class UPlayerHUDWidget;
class UTargetHUDWidget;
/**
 * 
 */
UCLASS()
class MAGEBOSSDUEL_API AMBDPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
    AMBDPlayerController();

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UPlayerHUDWidget> PlayerHUDWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UTargetHUDWidget> TargetHUDWidgetClass;

protected:
    virtual void BeginPlay() override;

    void ApplyMappingContexts();

    UPROPERTY()
    TObjectPtr<UPlayerHUDWidget> PlayerHUDWidget;

    UPROPERTY()
    TObjectPtr<UTargetHUDWidget> TargetHUDWidget;

public:
    UPROPERTY(EditDefaultsOnly, Category = "Input|Mapping")
    TObjectPtr<UInputMappingContext> IMC_Locomotion;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Mapping")
    int32 IMC_LocomotionPriority = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Mapping")
    TObjectPtr<UInputMappingContext> IMC_Combat;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Mapping")
    int32 IMC_CombatPriority = 1;
};
