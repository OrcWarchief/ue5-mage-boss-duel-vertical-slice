// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_FireChargedAttack.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Fire Charged Attack"))
class MAGEBOSSDUEL_API UAnimNotify_FireChargedAttack : public UAnimNotify
{
    GENERATED_BODY()

public:
    virtual void Notify(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference
    ) override;
};