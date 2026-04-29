#pragma once

#include "CoreMinimal.h"
#include "HitTypes.generated.h"

UENUM(BlueprintType)
enum class EHitReactionType : uint8
{
	None UMETA(DisplayName = "None"),

	LightStagger UMETA(DisplayName = "Light Stagger"),
	HeavyStagger UMETA(DisplayName = "Heavy Stagger"),
	Knockdown UMETA(DisplayName = "Knockdown")
};

USTRUCT(BlueprintType)
struct FHitPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Damage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PoiseDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	EHitReactionType ReactionType = EHitReactionType::LightStagger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	bool bCanInterrupt = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	bool bForceReaction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	bool bIgnorePoise = false;
};