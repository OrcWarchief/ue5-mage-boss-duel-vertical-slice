#include "Combat/CombatTargetFilter.h"

#include "Characters/Core/BaseCharacter.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

namespace MageBossDuel::CombatTargetFilter
{
namespace
{
	void AddIgnoredActorIfValid(FCollisionQueryParams& QueryParams, const AActor* Actor)
	{
		if (IsValid(Actor))
		{
			QueryParams.AddIgnoredActor(Actor);
		}
	}
}

ABaseCharacter* ResolveDamageCauser(const AActor* SourceActor)
{
	if (!IsValid(SourceActor))
	{
		return nullptr;
	}

	if (ABaseCharacter* OwnerCharacter = Cast<ABaseCharacter>(SourceActor->GetOwner()))
	{
		return OwnerCharacter;
	}

	if (ABaseCharacter* InstigatorCharacter = Cast<ABaseCharacter>(SourceActor->GetInstigator()))
	{
		return InstigatorCharacter;
	}

	return nullptr;
}

bool ShouldIgnoreActorForDamage(
	const AActor* Actor,
	const AActor* SourceActor,
	const AActor* DamageCauser
)
{
	if (!IsValid(Actor))
	{
		return true;
	}

	if (IsValid(SourceActor))
	{
		if (Actor == SourceActor)
		{
			return true;
		}

		if (Actor == SourceActor->GetOwner())
		{
			return true;
		}

		if (Actor == SourceActor->GetInstigator())
		{
			return true;
		}
	}

	if (IsValid(DamageCauser) && Actor == DamageCauser)
	{
		return true;
	}

	return false;
}

ABaseCharacter* GetAliveDamageTarget(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(Actor);
	if (!HitCharacter || !HitCharacter->IsAlive())
	{
		return nullptr;
	}

	return HitCharacter;
}

void AddIgnoredActorsForDamageQuery(
	FCollisionQueryParams& QueryParams,
	const AActor* SourceActor,
	const AActor* DamageCauser
)
{
	AddIgnoredActorIfValid(QueryParams, SourceActor);

	if (IsValid(SourceActor))
	{
		AddIgnoredActorIfValid(QueryParams, SourceActor->GetOwner());
		AddIgnoredActorIfValid(QueryParams, SourceActor->GetInstigator());
	}

	AddIgnoredActorIfValid(QueryParams, DamageCauser);
}
}