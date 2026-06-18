// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AnimNotify_FireChargedAttack.h"

#include "Characters/Player/PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_FireChargedAttack::Notify(
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

	APlayerCharacter* Player = Cast<APlayerCharacter>(MeshComp->GetOwner());

	if (!Player)
	{
		return;
	}

	Player->PerformChargedAttackRelease();
}

