// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BossFireball.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Boss Launch Fireball"))
class MAGEBOSSDUEL_API UAnimNotify_BossFireball : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	)override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Boss Launch Fireball");
	}
};
