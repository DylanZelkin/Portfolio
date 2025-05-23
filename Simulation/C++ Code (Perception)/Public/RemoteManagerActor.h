// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "EnvironmentActor.h"
#include "RemoteManagerActor.generated.h"

UCLASS()
class THESISSIMULATOR_API ARemoteManagerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARemoteManagerActor();
	virtual ~ARemoteManagerActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	FSocket* listenSocket;
	FSocket* clientSocket;

	TArray<AEnvironmentActor*> environments;
	int spectateEnvironmentID;

	UCameraComponent* spectatorCamera;


	// Called every frame
	virtual void Tick(float DeltaTime) override;


};
