// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MBDPlayerController.generated.h"

class UInputMappingContext;
class UPlayerHUDWidget;
class UTargetHUDWidget;
class UUserWidget;
/**
 * 
 */
UCLASS()
class MAGEBOSSDUEL_API AMBDPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
    AMBDPlayerController();
    void ShowRestPointActivatedNotice();

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UPlayerHUDWidget> PlayerHUDWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UTargetHUDWidget> TargetHUDWidgetClass;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void ApplyMappingContexts();

    UPROPERTY()
    TObjectPtr<UPlayerHUDWidget> PlayerHUDWidget;

    UPROPERTY()
    TObjectPtr<UTargetHUDWidget> TargetHUDWidget;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Rest Point")
    TSubclassOf<UUserWidget> RestPointActivatedWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Rest Point", meta = (ClampMin = "0.0", Units = "s"))
    float RestPointActivatedWidgetDuration = 2.0f;

public:
    UPROPERTY(EditDefaultsOnly, Category = "Input|Mapping")
    TObjectPtr<UInputMappingContext> IMC_Locomotion;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Mapping")
    int32 IMC_LocomotionPriority = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Mapping")
    TObjectPtr<UInputMappingContext> IMC_Combat;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Mapping")
    int32 IMC_CombatPriority = 1;

private:
    void HideRestPointActivatedNotice();

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> RestPointActivatedWidget = nullptr;

    FTimerHandle RestPointActivatedWidgetTimerHandle;
};
