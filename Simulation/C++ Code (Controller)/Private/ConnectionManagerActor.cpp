// Fill out your copyright notice in the Description page of Project Settings.



#include "ConnectionManagerActor.h"
#include "Sockets.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Common/TcpSocketBuilder.h"
#include <WinSock2.h>

// Sets default values
AConnectionManagerActor::AConnectionManagerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	clientSocket = nullptr;

	initialized = false;
	running_oy = 0;

	total_time = 0;
	num_ticks = 0;
}

AConnectionManagerActor::~AConnectionManagerActor()
{
	if (listenSocket != nullptr) {
		listenSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(listenSocket);
		//delete listenSocket;
	}
}

void AConnectionManagerActor::sendFPS()
{
	double frame_rate = num_ticks / total_time;

	total_time = 0;
	num_ticks = 0;
	
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetNumberField("FPS", frame_rate);

	//CODE 1 FOR 1 COPIED FROM EnvironmentManagerActor.cpp:
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	Writer->Close();

	if (!clientSocket || !clientSocket->GetConnectionState() == SCS_Connected) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Socket is not connected");
		return;
	}

	// Convert the JSON string to UTF-8 encoded data
	std::string Utf8JsonString = TCHAR_TO_UTF8(*JsonString);
	const uint8* JsonData = reinterpret_cast<const uint8*>(Utf8JsonString.c_str());
	int32 JsonDataLength = Utf8JsonString.length();

	// Prefix the length of the JSON string (4 bytes, network byte order)
	uint32 NetworkOrderLength = htonl(JsonDataLength);
	uint8 LengthBuffer[4];
	FMemory::Memcpy(LengthBuffer, &NetworkOrderLength, sizeof(uint32));

	// Send length prefix
	int32 BytesSent = 0;
	if (!clientSocket->Send(LengthBuffer, sizeof(LengthBuffer), BytesSent) || BytesSent != sizeof(LengthBuffer)) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to send message length");
		return;
	}

	// Send the JSON data
	if (!clientSocket->Send(JsonData, JsonDataLength, BytesSent) || BytesSent != JsonDataLength) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to send JSON data");
		return;
	}

	GEngine->ForceGarbageCollection(true);
}

// Called when the game starts or when spawned
void AConnectionManagerActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AConnectionManagerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!initialized) {
		initialized = true;
	}

	num_ticks += 1;
	total_time += DeltaTime;

	if (listenSocket == nullptr) {

		// setup the listener
		FTcpSocketBuilder* socketBuilder = new FTcpSocketBuilder(TEXT("Game Manager"));
		socketBuilder->AsReusable();
		socketBuilder->AsNonBlocking();
		socketBuilder->WithReceiveBufferSize(1024 * 64);
		FIPv4Address IPAddress(127, 0, 0, 1); // Bind to all available addresses
		socketBuilder->BoundToAddress(IPAddress);
		socketBuilder->BoundToPort(4567);
		listenSocket = socketBuilder->Build();
		delete socketBuilder;
		if (listenSocket && listenSocket->Listen(15)) {
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Listen Server Started");
		} else {
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Listen Server Failed to Start");
			return;
		}
	}

	int NumberOfEnvironments = environments.Num();
	// check for connection
	FSocket* newClient = listenSocket->Accept(TEXT("Client Connection"));
	if (newClient != nullptr) {
		if (clientSocket == nullptr) {
			clientSocket = newClient;
			return;
		}
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Client Connected!");
		//environment client
		double origin_x = 0;
		double origin_y = 200 + running_oy;
		double length;
		double width;
		double spawn_x_offset = 200;
		double spawn_y_offset;
		bool dynamicPlayer;
		FString envID;

		//GET SETUP MESSAGE:
			newClient->SetNonBlocking(false);

			uint8 ReceivedByte = 0;
			bool ReceivedBool = false;
			uint8 BoolBuffer = 0;
			int32 BytesRead = 0;

			// Read uint8
			if (newClient->Recv(&ReceivedByte, sizeof(uint8), BytesRead) && BytesRead == sizeof(uint8)) {
				if (ReceivedByte == 0) {
					envID = TEXT("Medium Box");
				} else if (ReceivedByte == 1) {
					envID = TEXT("Big Box");
				} else if (ReceivedByte == 2) {
					envID = TEXT("Square Box");
				} else {
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Didnt match envID");
					envID = TEXT("Medium Box");
				}
			} else {
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Server failed to get envID");
				envID = TEXT("Medium Box");
			}

			// Read bool (as a uint8, since UE5 sockets send raw bytes)
			if (newClient->Recv(&BoolBuffer, sizeof(uint8), BytesRead) && BytesRead == sizeof(uint8)) {
				ReceivedBool = BoolBuffer != 0;
				dynamicPlayer = ReceivedBool;
			} else {
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Server failed to get dynamicPlayer");
				dynamicPlayer = false;
			}

			newClient->SetNonBlocking(true);
		//END SETUP MESSAGE:


		
		if (envID == "Small Box") {
			length = 5;
			width = 5;
		} else if (envID == "Medium Box") {
			length = 15;
			width = 5;
		} else if (envID == "Big Box") {
			length = 25;
			width = 10;
		} else if (envID == "Square Box") {
			length = 30;
			width = 30;
		} else {
			length = 10;
			width = 10;
		}
		spawn_y_offset = width * 100 / 2;
		origin_y += width * 100;
		running_oy = origin_y;

		AEnvironmentManagerActor* EnvironmentManager = GetWorld()->SpawnActor<AEnvironmentManagerActor>(AEnvironmentManagerActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
		EnvironmentManager->initialize(newClient, origin_x, origin_y, length, width, spawn_x_offset, spawn_y_offset, dynamicPlayer);

		environments.Add(EnvironmentManager);
		//} else { //Bad environment... close connection, dont add }

	}

	//NOTE::
	// Same code as in: AEnvironmentManagerActor::Tick
	
	if (clientSocket != nullptr) {
		uint32 PendingDataSize;
		bool bSuccess = clientSocket->HasPendingData(PendingDataSize);
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("Controls has pending data %u"), PendingDataSize));
		if (!bSuccess) {
			// Error occurred, handle it
			int32 ErrorCode = FPlatformMisc::GetLastError();
			//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Couldnt Read From Client");
			return;
		} else {
			if (PendingDataSize <= 0) {
				return;
			}

			uint8 commandCode = 0;
			int32 BytesRead = 0;
			int32 TotalBytesRead = 0;

			while (TotalBytesRead < sizeof(commandCode)) {
				int32 CurrentBytesRead = 0;
				if (!clientSocket->Recv(reinterpret_cast<uint8*>(&commandCode) + TotalBytesRead,
					sizeof(commandCode) - TotalBytesRead,
					CurrentBytesRead) || CurrentBytesRead <= 0) {
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to receive commandCode (ConnectionManager)");
					return;
				}
				TotalBytesRead += CurrentBytesRead;
			}

			// Perform the command after successfully receiving commandCode
			if (commandCode == 0) { // Requesting State
				sendFPS();
			}else if (commandCode == 1) { // Setting Action
				//recieveAction();
			}
		}
	}
	
}

