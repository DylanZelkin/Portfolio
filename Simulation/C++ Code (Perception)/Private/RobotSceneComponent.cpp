// Fill out your copyright notice in the Description page of Project Settings.


#include "RobotSceneComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/BufferArchive.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include <Interfaces/ITargetPlatform.h>

void URobotSceneComponent::addToId2Label(int startIndex, int count, FString label) {
	int max_count = count;
	while (count > 0) {
		id2label.Add(startIndex + max_count - count, label);
		count--;
	}
}

void URobotSceneComponent::saveId2Label(const FString& FilePath) {
	// Convert TMap to JSON
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	for (const auto& Pair : id2label)
	{
		JsonObject->SetStringField(FString::FromInt(Pair.Key), Pair.Value);
	}

	// Convert JSON object to string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// Save the string to a file
	FFileHelper::SaveStringToFile(OutputString, *FilePath);
}

// Sets default values for this component's properties
URobotSceneComponent::URobotSceneComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
	recordingImage = false;
	recordingDepth = false;
	recordingSegment = false;

	Throttle = 0;
	Brake = 0;
	Steering = 0;

	running = true;
	firstTick = true;
	firstIncomingMessage = true;

	listenSocket = nullptr;
	listenPort = 4563;
	clientSocket = nullptr;

	current_spawn = 0;
	first_save = true;

	///Script/Engine.Material'/Game/Robots/PPM_Instance_Segmentation.PPM_Instance_Segmentation'
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Game/Robots/PPM_Instance_Segmentation"));
	if (MaterialFinder.Succeeded()) {
		SegmentationMaterial = MaterialFinder.Object;
	}
	else {
		SegmentationMaterial = nullptr;
	}

	//cameraConfig();

	//id2label for the lwst8 map (i know bad way to do this)... if project continues and more maps added, this process can be made automated
	// automated hint: set the hidden stencils at game start and set dict based on class type of objects in scene...
	//LABELS:
	id2label.Add(1, TEXT("floor-other-merged"));
	id2label.Add(2, TEXT("wall-other-merged"));
	id2label.Add(3, TEXT("wall-brick"));
	id2label.Add(4, TEXT("ceiling-merged"));
	id2label.Add(5, TEXT("window-other"));
	addToId2Label(10, 23, TEXT("door-stuff"));
	addToId2Label(35, 8, TEXT("table-merged"));
	addToId2Label(50, 5, TEXT("potted plant"));
	addToId2Label(60, 5, TEXT("tv"));
	addToId2Label(70, 3, TEXT("refrigerator"));
	addToId2Label(80, 4, TEXT("person"));
	addToId2Label(90, 2, TEXT("vase"));
	addToId2Label(100, 20, TEXT("chair"));
	addToId2Label(130, 4, TEXT("clock"));
	addToId2Label(140, 2, TEXT("laptop"));
	addToId2Label(150, 3, TEXT("light"));
	addToId2Label(160, 3, TEXT("cup"));

	FString FilePath = FPaths::ProjectDir() / TEXT("VisionData/id2label.json");
	saveId2Label(FilePath);
}

void URobotSceneComponent::cameraConfig() {
	Cameras[0] = FrontLeftCamera;
	Cameras[1] = FrontStraightCamera;
	Cameras[2] = FrontRightCamera;
	Cameras[3] = RearRightCamera;
	Cameras[4] = RearStraightCamera;
	Cameras[5] = RearLeftCamera;

	FString TextureName;
	UTextureRenderTarget2D* renderTarget;
	USceneCaptureComponent2D* cameraCopy;

	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("Camera Texture Format: %d"), Cameras[0]->TextureTarget->GetFormat()));

	TArray<FString> CameraSetupFile;
	CameraSetupFile.SetNum(num_cameras);

	for (int i = 0; i < num_cameras; i++) {
		int32 Width = 256;
		int32 Height = 256;
		TextureName = FString::Printf(TEXT("RobotImage_%d"), i);
		renderTarget = NewObject<UTextureRenderTarget2D>(this, *TextureName);// NewObject<UTextureRenderTarget2D>();
		renderTarget->InitAutoFormat(Width, Height);
		renderTarget->UpdateResourceImmediate();
		Cameras[i]->TextureTarget = renderTarget;

		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("Camera Texture Format: %d"), Cameras[0]->TextureTarget->GetFormat()));
		Cameras[i]->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
		Cameras[i]->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDRNoAlpha;
		Cameras[i]->bCaptureEveryFrame = false;
		Cameras[i]->bCaptureOnMovement = false;

		FTransform transform = Cameras[i]->GetRelativeTransform();
		FVector Translation = transform.GetLocation();
		FRotator Rotation = transform.GetRotation().Rotator();

		// Format the data as a string
		CameraSetupFile[i] = FString::Printf(TEXT("%f,%f,%f,%f,%f,%f"),
			Translation.X, Translation.Y, Translation.Z,
			Rotation.Pitch, Rotation.Yaw, Rotation.Roll);

		if (recordingDepth) {
			TextureName = FString::Printf(TEXT("DepthCamera_%d"), i);
			cameraCopy = NewObject<USceneCaptureComponent2D>(this, *TextureName);
			
			cameraCopy->FOVAngle = Cameras[i]->FOVAngle;
			cameraCopy->ProjectionType = Cameras[i]->ProjectionType;
			cameraCopy->bCaptureEveryFrame = Cameras[i]->bCaptureEveryFrame;
			cameraCopy->bCaptureOnMovement = Cameras[i]->bCaptureOnMovement;
			cameraCopy->SetRelativeTransform(Cameras[i]->GetRelativeTransform());
			cameraCopy->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
			cameraCopy->RegisterComponent();
			//.cameraCopy->post

			Depths[i] = cameraCopy;
			TextureName = FString::Printf(TEXT("RobotDepth_%d"), i);
			renderTarget = NewObject<UTextureRenderTarget2D>(this, *TextureName);// NewObject<UTextureRenderTarget2D>();
			renderTarget->InitAutoFormat(Width, Height);
			//renderTarget->OverrideFormat=(EPixelFormat::PF_R32_FLOAT);
			renderTarget->UpdateResourceImmediate();
			Depths[i]->TextureTarget = renderTarget;

			Depths[i]->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
			//Depths[i]->ProjectionType = ECameraProjectionMode::Perspective;
			Depths[i]->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
			
		}
		if (recordingSegment) {
			TextureName = FString::Printf(TEXT("SegmentationCamera_%d"), i);
			cameraCopy = NewObject<USceneCaptureComponent2D>(this, *TextureName);

			cameraCopy->FOVAngle = Cameras[i]->FOVAngle;
			cameraCopy->ProjectionType = Cameras[i]->ProjectionType;
			//TODO figure out why the f the segmentation doesnt work when capturing manually
			//cameraCopy->bCaptureEveryFrame = Cameras[i]->bCaptureEveryFrame;
			//cameraCopy->bCaptureOnMovement = Cameras[i]->bCaptureOnMovement;
			cameraCopy->SetRelativeTransform(Cameras[i]->GetRelativeTransform());
			cameraCopy->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
			cameraCopy->RegisterComponent();
			//.cameraCopy->post

			Segments[i] = cameraCopy;
			TextureName = FString::Printf(TEXT("RobotSegmentation_%d"), i);
			renderTarget = NewObject<UTextureRenderTarget2D>(this, *TextureName);// NewObject<UTextureRenderTarget2D>();
			renderTarget->InitAutoFormat(Width, Height);
			
			//int value = renderTarget->GetFormat();
			//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("RENDER TARGET FORMAT: %i"), value));

			renderTarget->UpdateResourceImmediate();
			Segments[i]->TextureTarget = renderTarget;

			Segments[i]->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
			//Segments[i]->ProjectionType = ECameraProjectionMode::Perspective;
			//ESceneCaptureSource::SCS_SceneColorHDR
			
			Segments[i]->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
			
			// Load the post-process material
			
			//static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Robots/PPM_Instance_Segmentation"));
			if (SegmentationMaterial != nullptr)
			{
				//Add the post-process material as a blendable
				Segments[i]->PostProcessSettings.AddBlendable(SegmentationMaterial, 1.0f);
				
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to load post-process material :c");
				//UE_LOG(LogTemp, Error, TEXT("Failed to load post-process material!"));
			}
			//Segments[i]->PostProcessSettings.AddBlendable()
			
		}
		
	}

	// Define the file path
	FString FilePath = FPaths::ProjectDir() + TEXT("camera_setup.txt");
	// Join the FString array into a single FString with a newline or delimiter
	FString DataToSave = FString::Join(CameraSetupFile, TEXT("\n"));
	// Save the FString to a file
	FFileHelper::SaveStringToFile(DataToSave, *FilePath);
}


// Called when the game starts
void URobotSceneComponent::BeginPlay()
{
	Super::BeginPlay();
	PrimaryComponentTick.TickInterval = 0.1f; //seconds
	image_capture_tick = 5; //every 5 ticks, capture... ie half second
	current_image_tick = 0;

	//cameraConfig();
	//Robot::startup(this);
	//Robot* robot = new Robot(this);
	//raphEventRef TaskResf = TGraphTask<Robot>::CreateTask(nullptr, ENamedThreads::GameThread).ConstructAndDispatchWhenReady(robot);
	
	//TODO dict save

}

void URobotSceneComponent::readCameras() {
	UTextureRenderTarget2D* TextureTarget;
	FTextureRenderTargetResource* RTResource;
	FString FilePath;
	TArray< FLinearColor > pixels;
	TArray<float> floatArray;

	FString folderName = FString::Printf(TEXT("VisionData/Trial_%d/"), current_spawn);

	for (int i = 0; i < num_cameras; i++) {
		Cameras[i]->CaptureScene();
		TextureTarget = Cameras[i]->TextureTarget;
		RTResource = TextureTarget->GameThread_GetRenderTargetResource();
		// Check if the render target resource is valid
		if (RTResource) {
			//FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);			// Define read pixel flags
			TArray<FColor> Pixels;										// Define an array to store the pixels
			if(!RTResource->ReadPixels(Pixels/*, ReadPixelFlags*/))				// Read the pixels from the texture target
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Camera Capture: Couldnt perform pixel read :c");
			FilePath = FPaths::ProjectDir() + folderName + TEXT("Camera_Images.bin");

			TArray<uint8> ByteArray;
			for (const FColor& Color : Pixels) {
				ByteArray.Add(Color.R);
				ByteArray.Add(Color.G);
				ByteArray.Add(Color.B);
			}
			if (recordingImage) {
				if (first_save) {
					FFileHelper::SaveArrayToFile(ByteArray, *FilePath); //overwrite
				}
				else {
					FFileHelper::SaveArrayToFile(ByteArray, *FilePath, &IFileManager::Get(), FILEWRITE_Append);
				}
			}
			//
			// CLIENT CONNECTION
			//
			if (clientSocket != nullptr) {
				//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Transmitting Image To Client");

				const uint8* Data = ByteArray.GetData();
				int32 BytesToSend = ByteArray.Num();

				int32 BytesSent = 0;
				while (!clientSocket->Send(Data, BytesToSend, BytesSent)) {
					//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Image Sent To Client - BYTES: %d"), BytesSent));
					//if we fail to send it we need to try again (had a bug where the images would get desynced for random failure lol)

				}

			}

		}
		else {
			// Handle the case where the render target resource is invalid
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Camera Capture: Couldnt perform pixel read :c");
			return;
		}
		if (recordingDepth) {
			Depths[i]->CaptureScene();
			TextureTarget = Depths[i]->TextureTarget;
			RTResource = TextureTarget->GameThread_GetRenderTargetResource();
			// Check if the render target resource is valid
			if (RTResource) {
				FReadSurfaceDataFlags ReadPixelFlags(RCM_MinMax);
				pixels.Empty();
				floatArray.Empty();
				if (!RTResource->ReadLinearColorPixels(pixels, ReadPixelFlags))				// Read the pixels from the texture target
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Camera Capture: Couldnt perform pixel read :c");
				int a = 0;
				FBufferArchive BufferArchive;
				for (const FLinearColor& Color : pixels) {
					floatArray.Add(Color.R);
					a++;
				}
				//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("Camera Depth Saving Count: %d"), a));
				//FBufferArchive BufferArchive;
				//BufferArchive << floatArray;
				for (float Value : floatArray)
				{
					BufferArchive << Value;
				}
				FilePath = FPaths::ProjectDir() + folderName + TEXT("Camera_Depths.bin");
				if (first_save) {
					FFileHelper::SaveArrayToFile(BufferArchive, *FilePath); //overwrite
				}
				else {
					FFileHelper::SaveArrayToFile(BufferArchive, *FilePath, &IFileManager::Get(), FILEWRITE_Append);
				}

			}
			else {
				// Handle the case where the render target resource is invalid
				//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Depth Capture: Couldnt perform pixel read :c");
				return;
			}
		}
		if (recordingSegment) {
			//Segments[i]->CaptureScene();
			TextureTarget = Segments[i]->TextureTarget;
			RTResource = TextureTarget->GameThread_GetRenderTargetResource();
			// Check if the render target resource is valid
			if (RTResource) {
				int a = 0;
				FBufferArchive BufferArchive;
				pixels.Empty();
				floatArray.Empty();
				FReadSurfaceDataFlags ReadPixelFlags(RCM_MinMax);
				if (!RTResource->ReadLinearColorPixels(pixels, ReadPixelFlags))				// Read the pixels from the texture target
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Camera Capture: Couldnt perform pixel read :c");
				for (const FLinearColor& Color : pixels) {
					floatArray.Add(Color.R);
					a++;
				}
				//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("Camera Depth Saving Count: %d"), a));
				//FBufferArchive BufferArchive;
				//BufferArchive << floatArray;
				for (float Value : floatArray)
				{
					BufferArchive << Value;
				}
				FilePath = FPaths::ProjectDir() + folderName + TEXT("Camera_Segmentation.bin");
				if (first_save) {
					FFileHelper::SaveArrayToFile(BufferArchive, *FilePath); //overwrite
					first_save = false; //change it here... consider that all will be recording at once
				}
				else {
					FFileHelper::SaveArrayToFile(BufferArchive, *FilePath, &IFileManager::Get(), FILEWRITE_Append);
				}
			}
			else {
				// Handle the case where the render target resource is invalid
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Camera Capture: Couldnt perform pixel read :c");
				//return;
			}



		}
	}

}

void URobotSceneComponent::controlVehicle() {
	//
	// CLIENT CONNECTION
	//
	if (clientSocket != nullptr) {
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Reading Controls From Client");

		int32 BytesRead = 0;
		// Define the size of the float in bytes
		const int32 FloatSize = sizeof(float);
		// Buffer to store the incoming bytes
		uint32 BufferSize = FloatSize*3;
		uint32 PendingDataSize;

		bool bSuccess = clientSocket->HasPendingData(PendingDataSize);
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("Controls has pending data %u"), PendingDataSize));
		if (!bSuccess){
			// Error occurred, handle it
			int32 ErrorCode = FPlatformMisc::GetLastError();
			//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Couldnt Read From Client in Robot Control Vehicle");
			return;
		}
		else {
			uint32 num_inputs;
			if (firstIncomingMessage == true) {
				const int32 IntSize = sizeof(int);
				const int32 BoolSize = sizeof(bool);
				BufferSize = IntSize + BoolSize; 
				num_inputs = PendingDataSize / BufferSize;
				if (num_inputs > 0) {
					TArray<uint8> Data;
					for (uint32 i = 0; i < num_inputs; i++) {
						BytesRead = 0;
						Data.Empty();
						Data.AddUninitialized(BufferSize);
						bSuccess = clientSocket->Recv(Data.GetData(), BufferSize, BytesRead);
						if (BytesRead != BufferSize) {
							GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Read the Wrong number of bytes in the robot initial message (for spawn and recording settings)");
							return;
						}
					}

					// Create a temporary byte array to hold the bytes
					TArray<uint8> DataSpawn;
					TArray<uint8> DataRecording;
					DataSpawn.Append(Data.GetData() + 0, IntSize);
					DataRecording.Append(Data.GetData() + IntSize, BoolSize);

					int spawn;
					bool recording;
					FMemory::Memcpy(&spawn, DataSpawn.GetData(), IntSize);
					FMemory::Memcpy(&recording, DataRecording.GetData(), BoolSize);

					current_spawn = spawn;

					//spawn the robot at spawnpoint
					
					FTransform transform = GetOwner()->GetActorTransform();
					FVector NewLocation;
					FQuat NewRotation;

					//rotations are actually _,z,_ instead of x,y,z from the editor... idk why
					if (spawn == 0) {
						//here1
						NewLocation = FVector(-19397.102091f, 12351.507778f, 20.523857f);
						NewRotation = FQuat(FRotator(0.0f, -84.0f, 0.0f));
					}
					else if (spawn == 1) {
						NewLocation = FVector(-19555.658392f, 11262.123767f, 20.023855f);
						NewRotation = FQuat(FRotator(0.0f, -175.999998f, 0.0f));
					}
					else if (spawn == 2) {
						NewLocation = FVector(-19682.815555f, 11121.563428f, 20.523857f);
						NewRotation = FQuat(FRotator(0.0f, -76.0f, 0.0f));
					}
					else if (spawn == 3) {
						NewLocation = FVector(-18946.412552f, 10530.943458f, 20.523857f);
						NewRotation = FQuat(FRotator(0.0f, -181.0f, 0.0f));
					}
					else if (spawn == 4) {
						NewLocation = FVector(-20960.14282f, 10682.609102f, 20.523857f);
						NewRotation = FQuat(FRotator(0.0f, -266.0f, 0.0f));
					}
					else if (spawn == 5) {
						NewLocation = FVector(-20907.770642f, 11278.788401f, 20.523857f);
						NewRotation = FQuat(FRotator(0.0f, -366.0f, 0.0f));
					}
					
					if (spawn == -1){
						//dont change it
					}
					else {
						GetOwner()->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);
					}
	
					//TODO set recording and call camera config
					if (recording == true) {
						recordingImage = true;
						recordingDepth = true;
						recordingSegment = true;
					}
					cameraConfig();
					firstIncomingMessage = false;

				}

			} else {
				num_inputs = PendingDataSize / BufferSize;
				//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("num_inputs %u"), num_inputs));

				if (num_inputs > 0) {

					TArray<uint8> Data;
					for (uint32 i = 0; i < num_inputs; i++) {
						BytesRead = 0;
						Data.Empty();
						Data.AddUninitialized(BufferSize);
						bSuccess = clientSocket->Recv(Data.GetData(), BufferSize, BytesRead);
						if (BytesRead != BufferSize) {
							//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Read the Wrong number of bytes in the robot control vehicle");
							return;
						}
					}

					// Create a temporary byte array to hold the bytes
					TArray<uint8> DataThrottle;
					TArray<uint8> DataBrake;
					TArray<uint8> DataSteering;
					DataThrottle.Append(Data.GetData() + 0, FloatSize);
					DataBrake.Append(Data.GetData() + FloatSize, FloatSize);
					DataSteering.Append(Data.GetData() + 2 * FloatSize, FloatSize);
					// Ensure little endian byte order 
					/* YOU DONT ACTUALLY NEED TO DO THIS, just make sure python is also sending little endian (mine is by default)
					if (!ITargetPlatform::IsLittleEndian())
					{
						Algo::Reverse(DataThrottle);
						Algo::Reverse(DataBrake);
						Algo::Reverse(DataSteering);
					}
					*/
					// Convert the byte array to a float
					float throttle;
					float brake;
					float steering;

					FMemory::Memcpy(&throttle, DataThrottle.GetData(), FloatSize);
					FMemory::Memcpy(&brake, DataBrake.GetData(), FloatSize);
					FMemory::Memcpy(&steering, DataSteering.GetData(), FloatSize);

					//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("Controls set: - thr: %f brk: %f str: %f"), throttle, brake, steering));

					Throttle = throttle;
					Brake = brake;
					Steering = steering;

				}
			}
		}

	}
}

// Called every frame
void URobotSceneComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) 
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Robot Running");
	if (firstTick) {
		//cameraConfig();
		firstTick = false;
		socketBuilder = new FTcpSocketBuilder(TEXT("TODO PUT SOCKET SERVER NAME HERE"));
		socketBuilder->AsReusable();
		socketBuilder->AsNonBlocking();
		FIPv4Address IPAddress(127, 0, 0, 1); // Bind to all available addresses
		socketBuilder->BoundToAddress(IPAddress);
		socketBuilder->BoundToPort(listenPort);
		listenSocket = socketBuilder->Build();
		if (listenSocket && listenSocket->Listen(1)) {
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Started client server successfully");
		} else {
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to start the client server");
		}
	}
	else {
		
		if (clientSocket == nullptr && listenSocket) {
			clientSocket = listenSocket->Accept(TEXT("ClientConnection"));
			if (clientSocket != nullptr) {
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Connection Accepted");
				//TODO first com

			}
		}else if (current_image_tick == image_capture_tick-1) {
			readCameras();
			current_image_tick = 0;
		}
		else {
			current_image_tick++;
		}
		
		controlVehicle();

	}
}

void URobotSceneComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Custom cleanup code here
	if (listenSocket)
	{
		listenSocket->Close();  // Close the socket
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(listenSocket);  // Destroy the socket
		listenSocket = nullptr;  // Set the pointer to nullptr
	}

	if (clientSocket)
	{
		clientSocket->Close();  // Close the socket
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(clientSocket);  // Destroy the socket
		clientSocket = nullptr;  // Set the pointer to nullptr
	}

}