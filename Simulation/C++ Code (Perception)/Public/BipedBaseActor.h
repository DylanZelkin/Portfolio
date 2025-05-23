// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Chaos/ChaosEngineInterface.h"

#include "BipedBaseActor.generated.h"

UCLASS()
class THESISSIMULATOR_API ABipedBaseActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// constructor
	ABipedBaseActor();
	virtual ~ABipedBaseActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Skeleton
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* SkeletalMeshComp;

	// Skeleton Data
	TMap<FString, FVector> ConstraintMap;
	void recordConstraints();
	TMap<FString, FTransform> TransformMap;
	void recordTransforms();

	// Agent Funcs
	/*void getState();
	void actionBuffer();
	void setActionBuffer();*/

	// local collision map
	TMap<FString, FString> CollisionMap;
	static TArray<ABipedBaseActor*> SelfReferences; //not using unreal's pointer system... (it just doesnt matter here)
	//static void OnContactModify(FPhysScene* PhysScene, PxContactModifyPair* ContactPairs, int32 NumPairs);
	/*static void OnOngoingCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse, const FHitResult& Hit);*/
};
