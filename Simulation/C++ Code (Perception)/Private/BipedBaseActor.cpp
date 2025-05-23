// Fill out your copyright notice in the Description page of Project Settings.


#include "BipedBaseActor.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

TArray<ABipedBaseActor*> ABipedBaseActor::SelfReferences;

// Sets default values
ABipedBaseActor::ABipedBaseActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Setup the SkeletalMeshComp
	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp"));
	RootComponent = SkeletalMeshComp;
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkeletalMeshAsset(TEXT("/Game/Path/To/Your/SkeletalMesh.SkeletalMesh"));
	if (SkeletalMeshAsset.Succeeded()) { SkeletalMeshComp->SetSkeletalMesh(SkeletalMeshAsset.Object); }
	else {}
	SkeletalMeshComp->SetSimulatePhysics(true);
	SkeletalMeshComp->SetWorldScale3D(FVector(0.2f, 0.2f, 0.2f));

    if (SelfReferences.Num() < 1) {
        //first actor added, bind the callbackfunction
        
    }
    SelfReferences.Add(this);
}

ABipedBaseActor::~ABipedBaseActor()
{
    SelfReferences.Remove(this);
    if (SelfReferences.Num() < 1) {
        //last actor removed, unbind the callbackfunction

    }
}


// Called when the game starts or when spawned
void ABipedBaseActor::BeginPlay()
{
	Super::BeginPlay();
	
}


void ABipedBaseActor::recordConstraints()
{
	UPhysicsAsset* PhysicsAsset = SkeletalMeshComp->GetPhysicsAsset();
	if (!PhysicsAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Physics Asset not found!"));
		return;
	}

    for (UPhysicsConstraintTemplate* ConstraintTemplate : PhysicsAsset->ConstraintSetup)
    {
        if (!ConstraintTemplate)
        {
            continue;
        }

        // Get the constraint instance
        FConstraintInstance* ConstraintInstance = SkeletalMeshComp->FindConstraintInstance(ConstraintTemplate->DefaultInstance.JointName);
        if (!ConstraintInstance)
        {
            continue;
        }

        // Get the current angles of the constraint
        float AngleSwing1 = ConstraintInstance->GetCurrentSwing1();
        float AngleSwing2 = ConstraintInstance->GetCurrentSwing2();
        float AngleTwist = ConstraintInstance->GetCurrentTwist();

        // Record the current angles
        ConstraintMap.Add(ConstraintTemplate->DefaultInstance.JointName.ToString(), FVector(AngleSwing1, AngleSwing2, AngleTwist));

        // TODO remove log
        /*UE_LOG(LogTemp, Log, TEXT("Constraint %s angles - Roll: %f, Pitch: %f, Yaw: %f"),
            *ConstraintTemplate->DefaultInstance.JointName.ToString(),
            AngleSwing1, AngleSwing2, AngleTwist);*/
    }

}

void ABipedBaseActor::recordTransforms()
{
    UPhysicsAsset* PhysicsAsset = SkeletalMeshComp->GetPhysicsAsset();
    if (!PhysicsAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("Physics Asset not found!"));
        return;
    }

    for (int32 BodyIndex = 0; BodyIndex < PhysicsAsset->SkeletalBodySetups.Num(); ++BodyIndex)
    {
        USkeletalBodySetup* BodySetup = PhysicsAsset->SkeletalBodySetups[BodyIndex];
        if (!BodySetup)
        {
            continue;
        }

        // Get the bone name for this body
        FName BoneName = BodySetup->BoneName;

        // Find the corresponding body instance in the skeletal mesh component
        FBodyInstance* BodyInstance = SkeletalMeshComp->GetBodyInstance(BoneName);
        if (!BodyInstance)
        {
            continue;
        }

        // Get the world transform of this body instance
        FTransform WorldTransform = BodyInstance->GetUnrealWorldTransform();

        TransformMap.Add(BoneName.ToString(), WorldTransform);
        // Log or use the position and rotation as needed
        /*UE_LOG(LogTemp, Log, TEXT("Body: %s, Position: X=%f, Y=%f, Z=%f, Rotation: Pitch=%f, Yaw=%f, Roll=%f"),
            *BoneName.ToString(),
            WorldPosition.X, WorldPosition.Y, WorldPosition.Z,
            WorldRotation.Pitch, WorldRotation.Yaw, WorldRotation.Roll);*/
    }


}

// Called every frame
void ABipedBaseActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    recordConstraints();
    recordTransforms();
}

