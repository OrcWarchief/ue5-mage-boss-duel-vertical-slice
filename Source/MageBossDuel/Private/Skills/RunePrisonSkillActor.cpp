// Fill out your copyright notice in the Description page of Project Settings.


#include "Skills/RunePrisonSkillActor.h"

#include "Combat/CombatTargetFilter.h"
#include "Characters/Core/BaseCharacter.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Skills/RunePrisonBeamSegment.h"
#include "TimerManager.h"

namespace CombatTargetFilter = MageBossDuel::CombatTargetFilter;

ARunePrisonSkillActor::ARunePrisonSkillActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Tags.Add(TEXT("BossSkillActor"));

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	InitialLifeSpan = 8.0f;

	BeamHitPayload.Damage = 10.0f;
	BeamHitPayload.PoiseDamage = 35.0f;
	BeamHitPayload.ReactionType = EHitReactionType::HeavyStagger;
	BeamHitPayload.bCanInterrupt = true;
	BeamHitPayload.bForceReaction = false;
	BeamHitPayload.bIgnorePoise = false;

	FinalBlastPayload.Damage = 35.0f;
	FinalBlastPayload.PoiseDamage = 100.0f;
	FinalBlastPayload.ReactionType = EHitReactionType::Knockdown;
	FinalBlastPayload.bCanInterrupt = true;
	FinalBlastPayload.bForceReaction = false;
	FinalBlastPayload.bIgnorePoise = false;
}

void ARunePrisonSkillActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActivateTimerHandle);
		World->GetTimerManager().ClearTimer(FinalBlastTimerHandle);
		World->GetTimerManager().ClearTimer(CleanupTimerHandle);
	}

	for (ARunePrisonBeamSegment* Segment : ActiveSegments)
	{
		if (IsValid(Segment))
		{
			Segment->Destroy();
		}
	}

	ActiveSegments.Reset();

	Super::EndPlay(EndPlayReason);
}

void ARunePrisonSkillActor::InitializePrison(ABaseCharacter* InDamageCauser, AActor* InTargetActor, const FVector& InPrisonCenter)
{
	DamageCauser = InDamageCauser;
	TargetActor = InTargetActor;

	FVector GroundCenter = InPrisonCenter;

	if (ProjectPointToGround(InPrisonCenter, GroundCenter))
	{
		PrisonCenter = GroundCenter;
	}
	else
	{
		PrisonCenter = InPrisonCenter;
	}

	SetActorLocation(PrisonCenter);

	const float RequiredLifeSpan =
		TelegraphDuration + ActiveDurationBeforeFinalBlast + CleanupDelayAfterFinalBlast + 1.0f;

	SetLifeSpan(FMath::Max(RequiredLifeSpan, 1.0f));

	const int32 ClampedRuneCount = FMath::Max(3, RuneCount);
	RuneCount = ClampedRuneCount;

	if (bOpenOneGap)
	{
		OpenGapIndex = FMath::RandRange(0, RuneCount - 1);
	}
	else
	{
		OpenGapIndex = INDEX_NONE;
	}

	BuildAnchorLocations();

	OnPrisonTelegraphStarted(AnchorLocations, OpenGapIndex, TelegraphDuration);

	UWorld* World = GetWorld();
	if (!World)
	{
		Destroy();
		return;
	}

	if (TelegraphDuration <= 0.0f)
	{
		ActivatePrison();
		return;
	}

	World->GetTimerManager().SetTimer(
		ActivateTimerHandle,
		this,
		&ARunePrisonSkillActor::ActivatePrison,
		TelegraphDuration,
		false
	);
}

void ARunePrisonSkillActor::ActivatePrison()
{
	if (bPrisonActive)
	{
		return;
	}

	bPrisonActive = true;

	SpawnBeamSegments();

	for (ARunePrisonBeamSegment* Segment : ActiveSegments)
	{
		if (IsValid(Segment))
		{
			Segment->ActivateSegment();
		}
	}

	OnPrisonActivated(AnchorLocations, OpenGapIndex);

	UWorld* World = GetWorld();
	if (!World)
	{
		TriggerFinalBlast();
		return;
	}

	if (ActiveDurationBeforeFinalBlast <= 0.0f)
	{
		TriggerFinalBlast();
		return;
	}

	World->GetTimerManager().SetTimer(
		FinalBlastTimerHandle,
		this,
		&ARunePrisonSkillActor::TriggerFinalBlast,
		ActiveDurationBeforeFinalBlast,
		false
	);
}

void ARunePrisonSkillActor::TriggerFinalBlast()
{
	if (bFinalBlastTriggered)
	{
		return;
	}

	bFinalBlastTriggered = true;

	const FVector BlastOrigin = PrisonCenter;
	const float BlastRadius = FinalBlastRadius;

	OnPrisonFinalBlast(BlastOrigin, BlastRadius);

	ApplyFinalBlastDamage();

	UWorld* World = GetWorld();
	if (!World)
	{
		CleanupPrison();
		return;
	}

	if (CleanupDelayAfterFinalBlast <= 0.0f)
	{
		CleanupPrison();
		return;
	}

	World->GetTimerManager().SetTimer(
		CleanupTimerHandle,
		this,
		&ARunePrisonSkillActor::CleanupPrison,
		CleanupDelayAfterFinalBlast,
		false
	);
}

void ARunePrisonSkillActor::CleanupPrison()
{
	OnPrisonFinished();

	for (ARunePrisonBeamSegment* Segment : ActiveSegments)
	{
		if (IsValid(Segment))
		{
			Segment->Destroy();
		}
	}

	ActiveSegments.Reset();

	Destroy();
}

void ARunePrisonSkillActor::BuildAnchorLocations()
{
	AnchorLocations.Reset();

	const int32 Count = FMath::Max(3, RuneCount);
	AnchorLocations.Reserve(Count);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float AngleRadians =
			2.0f * PI * static_cast<float>(Index) / static_cast<float>(Count);

		const FVector Offset(
			FMath::Cos(AngleRadians) * PrisonRadius,
			FMath::Sin(AngleRadians) * PrisonRadius,
			0.0f
		);

		FVector AnchorLocation = PrisonCenter + Offset;
		FVector GroundAnchor = AnchorLocation;

		if (ProjectPointToGround(AnchorLocation, GroundAnchor))
		{
			AnchorLocation = GroundAnchor;
		}

		AnchorLocations.Add(AnchorLocation);
	}
}

bool ARunePrisonSkillActor::ProjectPointToGround(const FVector& SourcePoint, FVector& OutGroundPoint) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector TraceStart = SourcePoint + FVector::UpVector * GroundTraceUp;
	const FVector TraceEnd = SourcePoint - FVector::UpVector * GroundTraceDown;

	FCollisionQueryParams QueryParams(
		FName(TEXT("RunePrisonGroundTrace")),
		false,
		this
	);

	if (AActor* CauserActor = DamageCauser.Get())
	{
		QueryParams.AddIgnoredActor(CauserActor);
	}

	FHitResult GroundHit;
	const bool bHit = World->LineTraceSingleByChannel(
		GroundHit,
		TraceStart,
		TraceEnd,
		GroundTraceChannel.GetValue(),
		QueryParams
	);

	if (!bHit)
	{
		return false;
	}

	OutGroundPoint = GroundHit.ImpactPoint;
	return true;
}

void ARunePrisonSkillActor::SpawnBeamSegments()
{
	UWorld* World = GetWorld();

	if (!World || !BeamSegmentClass)
	{
		return;
	}

	const int32 Count = FMath::Max(3, AnchorLocations.Num());

	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (Index == OpenGapIndex)
		{
			continue;
		}

		const int32 NextIndex = (Index + 1) % Count;

		const FVector SegmentStart =
			AnchorLocations[Index] + FVector::UpVector * (BeamHeight * 0.5f);

		const FVector SegmentEnd =
			AnchorLocations[NextIndex] + FVector::UpVector * (BeamHeight * 0.5f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		if (ABaseCharacter* Causer = DamageCauser.Get())
		{
			SpawnParams.Instigator = Causer;
		}

		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ARunePrisonBeamSegment* Segment = World->SpawnActor<ARunePrisonBeamSegment>(
			BeamSegmentClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (!Segment)
		{
			continue;
		}

		Segment->InitializeSegment(
			DamageCauser.Get(),
			SegmentStart,
			SegmentEnd,
			BeamThickness,
			BeamHeight,
			BeamHitPayload
		);

		ActiveSegments.Add(Segment);
	}
}

void ARunePrisonSkillActor::ApplyFinalBlastDamage()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ABaseCharacter* Causer = DamageCauser.Get();

	TArray<FOverlapResult> OverlapResults;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(
		FName(TEXT("RunePrisonFinalBlast")),
		false,
		this
	);

	if (AActor* CauserActor = DamageCauser.Get())
	{
		QueryParams.AddIgnoredActor(CauserActor);
	}

	const FCollisionShape BlastShape =
		FCollisionShape::MakeSphere(FinalBlastRadius);

	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		PrisonCenter,
		FQuat::Identity,
		ObjectQueryParams,
		BlastShape,
		QueryParams
	);

	if (!bHasOverlap)
	{
		return;
	}

	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Actor = Result.GetActor();

		if (CombatTargetFilter::ShouldIgnoreActorForDamage(Actor, this, Causer))
		{
			continue;
		}

		if (DamagedActors.Contains(Actor))
		{
			continue;
		}

		ABaseCharacter* HitCharacter = CombatTargetFilter::GetAliveDamageTarget(Actor);
		if (!HitCharacter)
		{
			continue;
		}

		DamagedActors.Add(Actor);

		HitCharacter->ApplyHitPayload(FinalBlastPayload, Causer);
	}
}