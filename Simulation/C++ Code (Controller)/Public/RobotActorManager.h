// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerActor.h"
#include "RobotActorManager.generated.h"

UCLASS()
class GAMEANIMATIONSAMPLE_API ARobotActorManager : public APlayerActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARobotActorManager();

	bool firstAction;
	virtual void Respawn(FVector SpawnLocation, FRotator SpawnRotation) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	TMap<FString, FVector> recordConstraints();
	void updateConstraints(TMap<FString, FVector> motor_controls);

	virtual TSharedPtr<FJsonObject> getState() override;
	virtual void setAction(TSharedPtr<FJsonObject> action) override;

	TArray<FString> FloorInvulnerableBodies;
	virtual void OnFloorHit(UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit) override;

	virtual void OnBarrierHit(UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit) override;
};
