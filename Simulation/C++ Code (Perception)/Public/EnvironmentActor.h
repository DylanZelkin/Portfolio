// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Common/TcpSocketBuilder.h"
#include "Camera/CameraComponent.h"
#include "RobotActor.h"
#include "EnvironmentActor.generated.h"

UCLASS()
class THESISSIMULATOR_API AEnvironmentActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnvironmentActor();
	virtual void initialize(FSocket* socket, int environmentNumber, int roomNumber, ARobotActor* new_robot);
	virtual ~AEnvironmentActor();

	FSocket* clientSocket;
	ARobotActor* robot;
	
	bool stepping;
	float actionTime;
	float maxActionTime;

	int environmentID;
	float origin_x;
	float origin_y;

	int roomID;
	float room_width;
	float room_length;
	int room_array_size;

	AActor* RoomActor;

	bool initialized;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/*virtual void spawnScene();
	virtual void reset();*/

	virtual void setCameraPose(UCameraComponent* spectatorCamera);
};
