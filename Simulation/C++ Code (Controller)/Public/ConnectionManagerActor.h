// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnvironmentManagerActor.h"
#include "ConnectionManagerActor.generated.h"

UCLASS()
class GAMEANIMATIONSAMPLE_API AConnectionManagerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AConnectionManagerActor();
	virtual ~AConnectionManagerActor();

	bool initialized;

	double running_oy;

	double num_ticks;
	double total_time;
	void sendFPS();

	FSocket* listenSocket;
	FSocket* clientSocket;
	TArray<AEnvironmentManagerActor*> environments;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	

};
