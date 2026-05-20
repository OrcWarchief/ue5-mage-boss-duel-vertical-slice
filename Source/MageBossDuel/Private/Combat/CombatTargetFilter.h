#pragma once

class AActor;
class ABaseCharacter;
struct FCollisionQueryParams;

namespace MageBossDuel::CombatTargetFilter
{
	ABaseCharacter* ResolveDamageCauser(const AActor* SourceActor);

	bool ShouldIgnoreActorForDamage(
		const AActor* Actor,
		const AActor* SourceActor,
		const AActor* DamageCauser = nullptr
	);

	ABaseCharacter* GetAliveDamageTarget(AActor* Actor);

	void AddIgnoredActorsForDamageQuery(
		FCollisionQueryParams& QueryParams,
		const AActor* SourceActor,
		const AActor* DamageCauser = nullptr
	);
}