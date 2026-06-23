// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BossBasicAttack.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Boss Basic Attack"))
class MAGEBOSSDUEL_API UAnimNotify_BossBasicAttack : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;
};
