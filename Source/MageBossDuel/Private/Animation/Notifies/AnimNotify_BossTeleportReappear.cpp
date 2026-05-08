// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AnimNotify_BossTeleportReappear.h"

#include "Characters/Boss/MageBossCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_BossTeleportReappear::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference
)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AMageBossCharacter* Boss = Cast<AMageBossCharacter>(MeshComp->GetOwner());
	if (!Boss)
	{
		return;
	}

	Boss->ReappearTeleport();
}