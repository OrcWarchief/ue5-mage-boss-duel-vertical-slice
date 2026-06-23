// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Notifies/AnimNotify_BossBasicAttack.h"
#include "Characters/Core/BaseCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_BossBasicAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(OwnerActor);
	if (!Character)
	{
		return;
	}

	Character->PerformBasicAttackHitCheck();
}
