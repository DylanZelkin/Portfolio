// Fill out your copyright notice in the Description page of Project Settings.

#include "EnvironmentActor.h"

// Sets default values
AEnvironmentActor::AEnvironmentActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	clientSocket = nullptr;
	stepping = false;
	actionTime = 0;

	initialized = false;
}

void AEnvironmentActor::initialize(FSocket* socket, int environmentNumber, int roomNumber, ARobotActor* new_robot)
{

	clientSocket = socket;
	environmentID = environmentNumber;
	robot = new_robot;
	roomID = roomNumber;

	FString RoomBlueprintPath;
	if (roomID == 0) {
		room_width = 2000.0;
		room_length = 2000.0;
		room_array_size = 4;
		RoomBlueprintPath = TEXT("/Game/Environments/RoomBlueprint.RoomBlueprint_C");
	}
	else {

	}

	UClass* RoomBlueprint = LoadObject<UClass>(nullptr, *RoomBlueprintPath);
	if (RoomBlueprint)
	{
		//UClass* RoomBlueprint = BlueprintClass.Object;

		int room_array_num = environmentID / room_array_size;
		int room_array_index = environmentID % room_array_size;

		// Set spawn location, rotation, and scale
		FVector SpawnLocation((float)(room_array_num * room_width) + origin_x, (float)(room_array_index * room_length) + origin_y, 0.0f);
		FRotator SpawnRotation(0.0f, 0.0f, 0.0f);
		FVector SpawnScale(10.0f, 10.0f, 5.0f);

		FTransform SpawnTransform(SpawnRotation, SpawnLocation);

		// Spawn the Blueprint actor
		RoomActor = GetWorld()->SpawnActor<AActor>(RoomBlueprint, SpawnTransform);
		RoomActor->SetActorScale3D(SpawnScale);

		if (RoomActor)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Successfully spawned the Room Actor.");
			UE_LOG(LogTemp, Log, TEXT("Successfully spawned the Room Actor."));
		}
		else
		{
			//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to spawn the Room Actor.");
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn the Room Actor."));
		}
	}
	else
	{
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Could not find the Room Blueprint.");
		UE_LOG(LogTemp, Warning, TEXT("Could not find the Room Blueprint."));
	}


	initialized = true;
}

AEnvironmentActor::~AEnvironmentActor()
{
}

// Called when the game starts or when spawned
void AEnvironmentActor::BeginPlay()
{
	Super::BeginPlay();
	//initialize();
}

// Called every frame
void AEnvironmentActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!initialized) {
		return;
	}

	/*if (!GetWorld()) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "World in tick failed");
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "World in tick failed");
	}*/

	if (this->stepping) {
		this->actionTime += DeltaTime;
		if (this->actionTime > this->maxActionTime) {

		}
		else {
			//wait...
		}
	}
	else {
		if (clientSocket != nullptr) {
			uint32 PendingDataSize;
			bool bSuccess = clientSocket->HasPendingData(PendingDataSize);
			//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("Controls has pending data %u"), PendingDataSize));
			if (!bSuccess) {
				// Error occurred, handle it
				int32 ErrorCode = FPlatformMisc::GetLastError();
				//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Couldnt Read From Client");
				return;
			}
			else {

			}

		}
		else {
			//not sure
		}
	}
}

void AEnvironmentActor::setCameraPose(UCameraComponent* spectatorCamera)
{
	int room_array_num = environmentID / room_array_size;
	int room_array_index = environmentID % room_array_size;

	if (roomID == 0) {
		// Set spawn location and rotation
		FVector SpawnLocation((float)(room_array_num * room_width) + origin_x, (float)(room_array_index * room_length) + origin_y + 350.0f, 1800.0f);
		//X,Z,Y here
		FRotator SpawnRotation(0.0f, 90.0f, -82.0f);

		// Apply the location and rotation to the spectator camera
		spectatorCamera->SetWorldLocation(SpawnLocation);
		spectatorCamera->SetWorldRotation(SpawnRotation);
	}
	else {

	}
}

