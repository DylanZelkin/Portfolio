// Fill out your copyright notice in the Description page of Project Settings.

#include "SimGameInstance.h"

void USimGameInstance::Init() {
    Super::Init();

    // Ensure we have a valid world
    UWorld* World = GetWorld();
    if (World)
    {
        // Define the spawn location and rotation
        FVector SpawnLocation(0.0f, 0.0f, 0.0f);
        FRotator SpawnRotation(0.0f, 0.0f, 0.0f);

        // Spawn the manager actor
        manager = World->SpawnActor<ARemoteManagerActor>(ARemoteManagerActor::StaticClass(), SpawnLocation, SpawnRotation);

        if (manager)
        {
            UE_LOG(LogTemp, Log, TEXT("Manager actor spawned successfully."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to spawn manager actor."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("World is null. Cannot spawn manager actor."));
    }
}

void USimGameInstance::Shutdown()
{
    Super::Shutdown();

    // Clean up manager if necessary
    if (manager)
    {
        manager->Destroy();
        manager = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Manager actor destroyed."));
    }
}