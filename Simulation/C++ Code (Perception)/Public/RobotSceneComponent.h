// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Common/TcpSocketBuilder.h"
#include "RobotSceneComponent.generated.h"



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THESISSIMULATOR_API URobotSceneComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URobotSceneComponent();

	void cameraConfig();
	void readCameras();

	void controlVehicle();
	
	void addToId2Label(int startIndex, int count, FString label);
	void saveId2Label(const FString& FilePath);


	FTcpSocketBuilder* socketBuilder;
	FSocket* listenSocket;
	FSocket* clientSocket;
	int32 listenPort;

	int image_capture_tick;
	int current_image_tick;

	TMap<int32, FString> id2label;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Destructor
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;	//its basically the same thing just covers more use cases

	bool running;
	bool firstTick;
	bool firstIncomingMessage;

	bool recordingImage;
	bool recordingDepth;
	bool recordingSegment;

	int current_spawn;
	bool first_save;

	UPROPERTY(EditAnywhere, Category = "Cameras")
	USceneCaptureComponent2D* FrontLeftCamera;
	UPROPERTY(EditAnywhere, Category = "Cameras")
	USceneCaptureComponent2D* FrontStraightCamera;
	UPROPERTY(EditAnywhere, Category = "Cameras")
	USceneCaptureComponent2D* FrontRightCamera;
	UPROPERTY(EditAnywhere, Category = "Cameras")
	USceneCaptureComponent2D* RearRightCamera;
	UPROPERTY(EditAnywhere, Category = "Cameras")
	USceneCaptureComponent2D* RearStraightCamera;
	UPROPERTY(EditAnywhere, Category = "Cameras")
	USceneCaptureComponent2D* RearLeftCamera;

	static const int num_cameras = 6;
	USceneCaptureComponent2D* Cameras[num_cameras];
	USceneCaptureComponent2D* Depths[num_cameras];
	USceneCaptureComponent2D* Segments[num_cameras];

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	float Throttle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	float Brake;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	float Steering;

	UPROPERTY()
	UMaterialInterface* SegmentationMaterial;
};
