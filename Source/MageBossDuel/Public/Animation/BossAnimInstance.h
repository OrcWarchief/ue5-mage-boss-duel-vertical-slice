// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BossAnimInstance.generated.h"

class AMageBossCharacter;

UENUM(BlueprintType)
enum class EBossMoveSide : uint8
{
    None  UMETA(DisplayName = "None"),
    Left  UMETA(DisplayName = "Left"),
    Right UMETA(DisplayName = "Right")
};
UCLASS()
class MAGEBOSSDUEL_API UBossAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Boss|Locomotion")
    TObjectPtr<AMageBossCharacter> BossOwner = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Boss|Locomotion")
    float Speed2D = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Boss|Locomotion")
    float LocalForwardSpeed = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Boss|Locomotion")
    float LocalRightSpeed = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Boss|Locomotion")
    bool bShouldMove = false;

    UPROPERTY(BlueprintReadOnly, Category = "Boss|Locomotion")
    bool bMoveLeft = false;

    UPROPERTY(BlueprintReadOnly, Category = "Boss|Locomotion")
    bool bMoveRight = false;

    UPROPERTY(BlueprintReadOnly, Category = "Boss|Locomotion")
    EBossMoveSide CurrentMoveSide = EBossMoveSide::None;

    UPROPERTY(BlueprintReadOnly, Category = "Boss|Locomotion")
    EBossMoveSide LastMoveSide = EBossMoveSide::Right;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Locomotion")
    float MoveSpeedThreshold = 15.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Locomotion")
    float SideSpeedThreshold = 10.0f;
};
