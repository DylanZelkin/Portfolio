// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "RobotCameraActor.generated.h"

UCLASS()
class GAMEANIMATIONSAMPLE_API ARobotCameraActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARobotCameraActor();
	bool initialized;

	ACameraActor* CameraActor;
	USceneCaptureComponent2D* SceneCapture;
	UTextureRenderTarget2D* RenderTarget;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	float timePassed;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	TSharedPtr<FJsonObject> Capture();
	void MoveTo(FVector TargetLocation, FRotator TargetRotation);

};
