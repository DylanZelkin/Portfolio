// Fill out your copyright notice in the Description page of Project Settings.

#include "EnvironmentManagerActor.h"
#include "CharacterManager.h"
#include "RobotActorManager.h"
#include <WinSock2.h>
#include "Sockets.h"
#include <PhysicsEngine/PhysicsConstraintActor.h>
#include <PhysicsEngine/PhysicsConstraintComponent.h>

// Sets default values
AEnvironmentManagerActor::AEnvironmentManagerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    initialized = false;
    player = nullptr;
}

void AEnvironmentManagerActor::initialize(FSocket* socket, double o_x, double o_y, double l, double w, double s_x_offset, double s_y_offset, bool dP)
{
    clientSocket = socket;
    origin_x = o_x;
    origin_y = o_y;
    length = l;
    width = w;
    spawn_x_offset = s_x_offset;
    spawn_y_offset = s_y_offset;
    dynamicPlayer = dP;
    height = 4.0;

    TimePassed = 0;

    if (!dynamicPlayer) {
        player = GetWorld()->SpawnActor<ACharacterManager>(ACharacterManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    } else {
        player = GetWorld()->SpawnActor<ARobotActorManager>(ARobotActorManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    }

    const FString BlueprintPath = TEXT("/Game/LevelPrototyping/LevelBlock.LevelBlock_C");
    UClass* BlueprintClass = LoadObject<UClass>(nullptr, *BlueprintPath);
    if (!BlueprintClass)
    {
        FString ErrorMessage = FString::Printf(TEXT("Failed to load Blueprint at path: %s"), *BlueprintPath);
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, ErrorMessage);
        return;
    }

    // Specify spawn location and rotation
    FVector SpawnLocation = FVector(origin_x, origin_y, 0.f); // Adjust as needed
    FRotator SpawnRotation = FRotator::ZeroRotator;
    FVector SpawnScale = FVector(1.0f, 1.0f, 1.0f);

    FVector AddSpawnLocation = FVector::ZeroVector;
    //4 walls
    for (int i = 0; i < 4; i++) {

        int x = i / 2;
        int y = i % 2;
        if (x == 0 && y == 0) {
            AddSpawnLocation = FVector(-10.f, 0.f, 0.f);
            SpawnScale = FVector(0.1f, width * 1.0f, height);
        } else if (x == 1 && y == 1) {
            AddSpawnLocation = FVector(0, -10, 0.f);
            SpawnScale = FVector(length * 1.0f, 0.1f, height);
        } else if (x == 0 && y == 1) {
            AddSpawnLocation = FVector(length * 100.f, 0, 0.f);
            SpawnScale = FVector(0.1f, width * 1.0f, height);
        } else if (x == 1 && y == 0) {
            AddSpawnLocation = FVector(0, width * 100.f, 0.f);
            SpawnScale = FVector(length * 1.0f, 0.1f, height);
        }
        FVector NewSpawnLocation = SpawnLocation + AddSpawnLocation;
        FTransform ActorTransform(SpawnRotation, NewSpawnLocation, SpawnScale);

        AActor* SpawnedActor = GetWorld()->SpawnActorDeferred<AActor>(BlueprintClass, ActorTransform, nullptr, nullptr);
        if (SpawnedActor) {
            // Set the 'ColorGroup' property before finishing spawning
            FName ColorGroupValue = FName(TEXT("Blocks_Traversable")); // Example value
            FProperty* Property = SpawnedActor->GetClass()->FindPropertyByName(TEXT("ColorGroup"));

            if (Property && Property->IsA(FNameProperty::StaticClass())) {
                FNameProperty* NameProperty = CastField<FNameProperty>(Property);
                if (NameProperty) {
                    NameProperty->SetPropertyValue_InContainer(SpawnedActor, ColorGroupValue);
                } else {
                    GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to find NameProperty");
                    return;
                }
            } else {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to find Property");
                return;
            }

            // Finalize the spawning process
            SpawnedActor->FinishSpawning(ActorTransform);
            
            UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(SpawnedActor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
            if (StaticMeshComp) {

                StaticMeshComp->SetNotifyRigidBodyCollision(true);
                StaticMeshComp->SetGenerateOverlapEvents(true);
                StaticMeshComp->SetSimulatePhysics(false);
                StaticMeshComp->SetMobility(EComponentMobility::Stationary);
                StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                StaticMeshComp->SetCollisionObjectType(ECC_WorldStatic); // Ensure it's detected properly
                StaticMeshComp->SetCollisionResponseToAllChannels(ECR_Block); // Adjust based on your needs
                StaticMeshComp->BodyInstance.SetUseCCD(true);
                StaticMeshComp->OnComponentHit.AddDynamic(player, &APlayerActor::OnBarrierHit);

                if (!StaticMeshComp->IsCollisionEnabled())
                    GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Barrier fail2");
            } else {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Barrier fail");
            }

            
            //GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("Collision is %d"), SpawnedActor->IsRootComponentCollisionRegistered()));
            //GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Spawned Barrier");
        } else {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to Spawn Barrier");
            return;
        }
        BarrierActors.Add(SpawnedActor);
    }

    SpawnScale = FVector(length * 1.0f, width * 1.0f, 0.1f);
    AddSpawnLocation = FVector(0, 0, -10.0f);
    FTransform ActorTransform(SpawnRotation, SpawnLocation+AddSpawnLocation, SpawnScale);
    Floor = GetWorld()->SpawnActorDeferred<AActor>(BlueprintClass, ActorTransform, nullptr, nullptr);
    if (Floor) {
        // Set the 'ColorGroup' property before finishing spawning
        FName ColorGroupValue = FName(TEXT("Landscape")); // Example value
        FProperty* Property = Floor->GetClass()->FindPropertyByName(TEXT("ColorGroup"));

        if (Property && Property->IsA(FNameProperty::StaticClass())) {
            FNameProperty* NameProperty = CastField<FNameProperty>(Property);
            if (NameProperty) {
                NameProperty->SetPropertyValue_InContainer(Floor, ColorGroupValue);
            } else {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to find NameProperty");
                return;
            }
        } else {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to find Property");
            return;
        }
        Floor->FinishSpawning(ActorTransform);
        UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Floor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
        if (StaticMeshComp) {
            StaticMeshComp->SetNotifyRigidBodyCollision(true);
            StaticMeshComp->SetGenerateOverlapEvents(true);
            StaticMeshComp->SetSimulatePhysics(false);
            StaticMeshComp->SetMobility(EComponentMobility::Stationary);
            StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            StaticMeshComp->SetCollisionObjectType(ECC_WorldStatic); // Ensure it's detected properly
            StaticMeshComp->SetCollisionResponseToAllChannels(ECR_Block); // Adjust based on your needs
            StaticMeshComp->BodyInstance.SetUseCCD(true);
            StaticMeshComp->OnComponentHit.AddDynamic(player, &APlayerActor::OnFloorHit);
        }
    }
    

    GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Env initialized");
    initialized = true;
}

void AEnvironmentManagerActor::OnBarrierHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    player->GetFName();
    OtherComp->GetFName();
    FName BodyName = OtherActor->GetFName();

    /*GEngine->AddOnScreenDebugMessage(
        -1, 5.0f, FColor::Yellow,
        FString::Printf(TEXT("Hit Body Name: %s"), *BodyName.ToString())
    );*/

    if (OtherActor->GetFName() == player->SkeletalMeshComp->GetFName()) {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Done do to barrier collision");
    }

    if (OtherComp && OtherComp == player->SkeletalMeshComp) {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Done do to barrier collision");
    }
}

// Called when the game starts or when spawned
void AEnvironmentManagerActor::BeginPlay()
{
    Super::BeginPlay();
}

// Called every frame
void AEnvironmentManagerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (!initialized) {
        return;
    }

    TimePassed += DeltaTime;

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
                        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to receive commandCode (EnvManager)");
                        return;
                }
                TotalBytesRead += CurrentBytesRead;
            }

            // Perform the command after successfully receiving commandCode
            if (commandCode == 0) { // Requesting State
                sendState();
            } else if (commandCode == 1) { // Setting Action
                recieveAction();
            }
        }

    } else {
        //not sure
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "client socket == nullptr (EnvManager)");
    }
}

bool AEnvironmentManagerActor::isDone()
{
    return player->isDone;
}

void AEnvironmentManagerActor::reset()
{   
    FVector SpawnLocation(origin_x + spawn_x_offset, origin_y + spawn_y_offset, 0.0f);
    player->Respawn(SpawnLocation, FRotator::ZeroRotator);

    TimePassed = 0;
}

void AEnvironmentManagerActor::sendState() {
    TSharedPtr<FJsonObject> JsonObject = player->getState();

    bool done = isDone();
    JsonObject->SetBoolField("Done", done);
    JsonObject->SetNumberField("TimePassed", TimePassed);
    TimePassed = 0;
    

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    Writer->Close();

    if (!clientSocket || !clientSocket->GetConnectionState() == SCS_Connected){
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
    if (!clientSocket->Send(LengthBuffer, sizeof(LengthBuffer), BytesSent) || BytesSent != sizeof(LengthBuffer)){
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to send message length");
        return;
    }

    // Send the JSON data
    if (!clientSocket->Send(JsonData, JsonDataLength, BytesSent) || BytesSent != JsonDataLength){
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to send JSON data");
        return;
    }
}

void AEnvironmentManagerActor::recieveAction() {
    // ReceiveJsonData
    uint8 LengthBuffer[4];
    int32 TotalLengthBytesRead = 0;

    // Ensure we read exactly 4 bytes for the length prefix
    while (TotalLengthBytesRead < 4) {
        int32 CurrentBytesRead = 0;
        if (!clientSocket->Recv(LengthBuffer + TotalLengthBytesRead, 4 - TotalLengthBytesRead, CurrentBytesRead) || CurrentBytesRead <= 0) {
            //GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to read message length");
            //return;
        }
        TotalLengthBytesRead += CurrentBytesRead;
    }

    // Convert length prefix to an integer
    int32 MessageLength = 0;
    FMemory::Memcpy(&MessageLength, LengthBuffer, sizeof(uint32));
    MessageLength = ntohl(MessageLength); // Convert from network byte order

    // Read the actual message with a loop to ensure all data is received
    TArray<uint8> MessageData;
    MessageData.Empty();
    MessageData.SetNumZeroed(MessageLength);  // Ensures no leftover data
    
    int32 TotalBytesRead = 0;
    while (TotalBytesRead < MessageLength) {
        int32 CurrentBytesRead = 0;
        if (!clientSocket->Recv(MessageData.GetData() + TotalBytesRead, MessageLength - TotalBytesRead, CurrentBytesRead) || CurrentBytesRead <= 0) {
            //GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to read full message data");
            //return;
        }
        TotalBytesRead += CurrentBytesRead;
    }

    MessageData.Add(0); // Null-terminate the buffer
    FString JsonReceived = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(MessageData.GetData())));
    TMap<FString, float> Result;
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonReceived);
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid()) {
        //GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, "Successfully parsed JSON.");
    } else {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to parse JSON. !!!<Environment Potentially Out of Sync with Server and Halting>!!!");
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "Received JSON: " + JsonReceived);
        //UE_LOG(LogTemp, Warning, TEXT("Received JSON: %s"), *JsonReceived);
        return;
    }

    bool Reset = false;
    if (JsonObject->HasField("Reset")) {
        JsonObject->TryGetBoolField("Reset", Reset);
    } else {
        //GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Didnt recieve field 'Reset' in player action");
    }

    if (Reset == true) {
        //GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "Resetting Enviornment");
        reset();
    } else {
        if(player->initialized)
            player->setAction(JsonObject);
        else
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "Attempting to perform action when player is not yet setup...");
    }
}