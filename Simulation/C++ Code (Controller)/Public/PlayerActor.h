// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerActor.generated.h"

struct FCollisionData
{
	UPrimitiveComponent* HitComponent;
	AActor* OtherActor;
	UPrimitiveComponent* OtherComp;
	FVector NormalImpulse;
	FHitResult Hit;

	FCollisionData(
		UPrimitiveComponent* InHitComponent,
		AActor* InOtherActor,
		UPrimitiveComponent* InOtherComp,
		const FVector& InNormalImpulse,
		const FHitResult& InHit)
		: HitComponent(InHitComponent),
		OtherActor(InOtherActor),
		OtherComp(InOtherComp),
		NormalImpulse(InNormalImpulse),
		Hit(InHit)
	{}

	FString ToString() const
	{
		return FString::Printf(
			TEXT("HitComponent: %s, OtherActor: %s, OtherComp: %s, NormalImpulse: %s, HitLocation: %s"),
			*HitComponent->GetName(),
			*OtherActor->GetName(),
			OtherComp ? *OtherComp->GetName() : TEXT("None"),
			*NormalImpulse.ToString(),
			*Hit.Location.ToString()
		);
	}
};

UCLASS(abstract)
class GAMEANIMATIONSAMPLE_API APlayerActor : public AActor
{
	GENERATED_BODY()


public:	
	// Sets default values for this actor's properties
	APlayerActor();

	USkeletalMeshComponent* SkeletalMeshComp;

	FTransform OriginInverse;
	TArray<FString> TransformBones;
	TMap<FString, FTransform> recordTransforms();

	TArray<AActor*> EnvironmentBarriers;
	TArray<FCollisionData> CollisionDataList;

	UFUNCTION()
	virtual void OnFloorHit(UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION()
	virtual void OnBarrierHit(UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit) PURE_VIRTUAL(APlayerActor::OnBarrierHit, );

	bool initialized;
	bool isDone;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void Respawn(FVector SpawnLocation, FRotator SpawnRotation);

	virtual TSharedPtr<FJsonObject> getState();
	virtual void setAction(TSharedPtr<FJsonObject> action) PURE_VIRTUAL(APlayerActor::setAction, );

};
