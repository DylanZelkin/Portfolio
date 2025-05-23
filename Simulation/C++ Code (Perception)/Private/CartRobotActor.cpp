// Fill out your copyright notice in the Description page of Project Settings.


#include "CartRobotActor.h"

ACartRobotActor::ACartRobotActor()
{
    meshPath = TEXT("SkeletalMesh'/Game/Path/To/YourSkeletalMesh.YourSkeletalMesh'");
}

void ACartRobotActor::initialize()
{
    SpawnScale = FVector(3.0f, 3.0f, 3.0f);

    // Load the skeletal mesh asset.
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkeletalMeshAsset(TEXT("SkeletalMesh'/Game/Path/To/YourSkeletalMesh.YourSkeletalMesh'"));
    if (SkeletalMeshAsset.Succeeded())
    {
        USkeletalMesh* SkeletalMesh = SkeletalMeshAsset.Object;

        // Create a skeletal mesh component dynamically.
        SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp"));
        SkeletalMeshComp->SetupAttachment(RootComponent);

        // Set the skeletal mesh to the component.
        if (SkeletalMesh)
        {
            SkeletalMeshComp->SetSkeletalMesh(SkeletalMesh);
        }

    }
    else {

    }
    Super::initialize();
}

void ACartRobotActor::getState()
{
    // Transforms
    //  Body

    // Constraints (Speed and Pos)
    //  Steer_FL
    //  Steer_FR
    //  Steer_BL
    //  Steer_BR
    //
    //  Wheel_FL
    //  Wheel_FR
    //  Wheel_BL
    //  Wheel_BR
    
}

void ACartRobotActor::setAction()
{
    // Constraints (Speed)
    //  Steer_FL
    //  Steer_FR
    //  Steer_BL
    //  Steer_BR
    //
    //  Wheel_FL
    //  Wheel_FR
    //  Wheel_BL
    //  Wheel_BR
}
