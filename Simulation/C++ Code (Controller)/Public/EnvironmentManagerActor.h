// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerActor.h"
#include "EnvironmentManagerActor.generated.h"

UCLASS()
class GAMEANIMATIONSAMPLE_API AEnvironmentManagerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnvironmentManagerActor();

	bool initialized;
	void initialize(FSocket* socket, double origin_x, double origin_y, double length, double width, double spawn_x_offset, double spawn_y_offset, bool dynamicPlayer);

	FSocket* clientSocket;

	bool dynamicPlayer;
	APlayerActor* player;

	// Array to store the spawned actor instances
	UPROPERTY()
	TArray<AActor*> BarrierActors;
	AActor* Floor;

	double origin_x;
	double origin_y;

	double length;
	double width;
	double height;

	double spawn_x_offset;
	double spawn_y_offset;

	UFUNCTION()
	void OnBarrierHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool isDone();
	virtual void reset();

	double TimePassed;
	void sendState();
	void recieveAction();
};
