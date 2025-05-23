// Fill out your copyright notice in the Description page of Project Settings.


#include "RemoteManagerActor.h"
#include "Common/TcpSocketBuilder.h"

// Sets default values
ARemoteManagerActor::ARemoteManagerActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	clientSocket = nullptr;

	// Setup the Camera
	spectatorCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	spectatorCamera->SetupAttachment(RootComponent);
}

ARemoteManagerActor::~ARemoteManagerActor()
{
	if (listenSocket != nullptr) {
		listenSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(listenSocket);
		//delete listenSocket;
	}
}

// Called when the game starts or when spawned
void ARemoteManagerActor::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ARemoteManagerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (listenSocket == nullptr) {

		// setup the listener
		FTcpSocketBuilder* socketBuilder = new FTcpSocketBuilder(TEXT("Game Manager"));
		socketBuilder->AsReusable();
		socketBuilder->AsNonBlocking();
		FIPv4Address IPAddress(127, 0, 0, 1); // Bind to all available addresses
		socketBuilder->BoundToAddress(IPAddress);
		socketBuilder->BoundToPort(4567);
		listenSocket = socketBuilder->Build();
		delete socketBuilder;
		if (listenSocket && listenSocket->Listen(15)) {
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Listen Server Started");
		}
		else {
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Listen Server Failed to Start");
			return;
		}
	}

	int NumberOfEnvironments = environments.Num();
	// check for connection
	FSocket* newClient = listenSocket->Accept(TEXT("Client Connection"));
	if (newClient != nullptr) {
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Client Connected!");

		if (clientSocket == nullptr) {
			//spectator client
			clientSocket = newClient;
			
		}
		else {
			//environment client
			int envID = 0;
			int robotID = 0;
			
			
			if (envID == 0) {
				int roomNumber=0;
				ARobotActor* new_robot = nullptr;

				FVector SpawnLocation(0.0f, 0.0f, 0.0f);
				FRotator SpawnRotation(0.0f, 0.0f, 0.0f);
				
				AEnvironmentActor* EnvironmentActor = GetWorld()->SpawnActor<AEnvironmentActor>(AEnvironmentActor::StaticClass(), SpawnLocation, SpawnRotation);
				EnvironmentActor->initialize(newClient, NumberOfEnvironments, roomNumber, new_robot);
				environments.Add(EnvironmentActor);
			}
			else {
				//Bad environment... close connection, dont add
			}
		}
			
	}

	if (clientSocket != nullptr) {
		//spectator client connected...
		int32 desiredSpectateEnvironmentID = 0;
		if (desiredSpectateEnvironmentID >= NumberOfEnvironments || desiredSpectateEnvironmentID < 0) {
			// invalid environment
		}
		else {
			// move camera
		}
		// snapshot and send the camera
	}

}

