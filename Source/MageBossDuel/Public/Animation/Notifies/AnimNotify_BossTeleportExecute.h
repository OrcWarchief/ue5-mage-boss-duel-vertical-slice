// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BossTeleportExecute.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Boss Teleport Execute"))
class MAGEBOSSDUEL_API UAnimNotify_BossTeleportExecute : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Boss Teleport Execute");
	}
};
