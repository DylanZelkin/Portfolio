// Fill out your copyright notice in the Description page of Project Settings.

#include "GameInstanceManager.h"
#include "FloorPlanGenerator.h"
#include "EnvironmentObjectActor.h"
#include "RobotCameraActor.h"

void UGameInstanceManager::Init() {
    Super::Init();

    // Ensure we have a valid world
    UWorld* World = GetWorld();
    if (World){
        /*AFloorPlanGenerator* floorplan = World->SpawnActor<AFloorPlanGenerator>(AFloorPlanGenerator::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        floorplan->initialize();*/
        ARobotCameraActor* camera = World->SpawnActor<ARobotCameraActor>(ARobotCameraActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        AEnvironmentObjectActor* object = World->SpawnActor<AEnvironmentObjectActor>(AEnvironmentObjectActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);

        ConnectionManager = World->SpawnActor<AConnectionManagerActor>(AConnectionManagerActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        if (ConnectionManager) {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Spawned Server");
        }else {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to spawn server");
        }
    } else{
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "World is null. Cannot spawn.");
    }

}