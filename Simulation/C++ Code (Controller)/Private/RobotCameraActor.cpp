// Fill out your copyright notice in the Description page of Project Settings.

#include "RobotCameraActor.h"
#include "Camera/CameraActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"


// Sets default values
ARobotCameraActor::ARobotCameraActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	initialized = false;
    timePassed = 0;
}

// Called when the game starts or when spawned
void ARobotCameraActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARobotCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!initialized) {
		UWorld* World = GetWorld();

		FActorSpawnParameters SpawnParams;
		CameraActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FVector(0, 0, 300), FRotator(-90, 0, 0), SpawnParams);
		SceneCapture = NewObject<USceneCaptureComponent2D>(CameraActor);
		
		SceneCapture->SetupAttachment(CameraActor->GetRootComponent());
		SceneCapture->RegisterComponent();

		// Create a Render Target
		RenderTarget = NewObject<UTextureRenderTarget2D>();
		RenderTarget->InitAutoFormat(512, 512);
		SceneCapture->TextureTarget = RenderTarget;

		initialized = true;
	}
    timePassed += DeltaTime;
    if (timePassed > 10) {
        
    }

}

TSharedPtr<FJsonObject> ARobotCameraActor::Capture()
{
    // Capture Scene
    SceneCapture->CaptureScene();

    // Read Pixels from Render Target
    FRenderTarget* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
    TArray<FColor> Bitmap;
    FIntPoint Size(RenderTarget->SizeX, RenderTarget->SizeY);

    if (RenderTargetResource && RenderTargetResource->ReadPixels(Bitmap)) {
        // Convert Image to PNG
        TArray<uint8> PNGData;
        FImageUtils::CompressImageArray(Size.X, Size.Y, Bitmap, PNGData);

        // Convert PNG to Base64
        FString Base64String = FBase64::Encode(PNGData);

        // Create JSON Object
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
        JsonObject->SetStringField("ImageBase64", Base64String);

        // Serialize JSON
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

        return JsonObject;
        // Save to File
        /*FString FilePath = FPaths::ProjectDir() + TEXT("CameraSnapshot.json");
        if (FFileHelper::SaveStringToFile(OutputString, *FilePath)) {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Snapshot saved to CameraSnapshot.json"));
        }
        else {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to save CameraSnapshot.json"));
        }*/
    }
    return NULL;
}

void ARobotCameraActor::MoveTo(FVector TargetLocation, FRotator TargetRotation)
{
    CameraActor->SetActorLocation(TargetLocation);
    CameraActor->SetActorRotation(TargetRotation);
}

