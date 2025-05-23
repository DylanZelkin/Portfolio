// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RobotActor.generated.h"

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
};

UCLASS()
class THESISSIMULATOR_API ARobotActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARobotActor();
	
	virtual void initialize();
	bool initialized;

	FString meshPath;

	uint32 stateSize;
	uint32 actionSize;

	FVector SpawnScale;
	FVector SpawnOffset;
	FVector SpawnRotation;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Skeleton
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* SkeletalMeshComp;

	TMap<FString, FVector> ConstraintMap;
	void recordConstraints();
	TMap<FString, FTransform> TransformMap;
	void recordTransforms();

	TArray<FCollisionData> CollisionDataList;
	void OnSkeletalMeshHit(UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	
	/*virtual void setAction();
	virtual void getState();*/
};
