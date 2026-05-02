// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/HitTypes.h"
#include "FireballProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class ABaseCharacter;

UCLASS()
class MAGEBOSSDUEL_API AFireballProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AFireballProjectile();
	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Skill|Fireball")
	void Explode(AActor* DirectHitActor);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement = nullptr;

	// ===== HitPayload =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Fireball|Hit")
	FHitPayload DirectHitPayload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Fireball|Hit")
	FHitPayload SplashHitPayload;

	// ===== Expolsion =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Fireball|Explosion", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float ExplosionRadius = 220.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Fireball|Debug")
	bool bDrawDebugExplosion = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Fireball|Runtime")
	bool bHasExploded = false;

	UFUNCTION()
	void OnProjectileBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& HitResult
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|Fireball")
	void OnFireballExploded(const FVector& ExplosionOrigin, AActor* DirectHitActor);

private:
	bool IsIgnoredActor(AActor* Actor) const;
	ABaseCharacter* GetDamageCausor() const;
	void ApplyExplosionDamage(AActor* DirectHitActor, const FVector& ExplosionOrigin);
};
