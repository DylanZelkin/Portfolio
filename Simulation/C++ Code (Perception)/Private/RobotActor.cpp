// Fill out your copyright notice in the Description page of Project Settings.


#include "RobotActor.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

// Sets default values
ARobotActor::ARobotActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    initialized = false;

    SpawnScale = FVector(1.0f, 1.0f, 1.0f);
    SpawnOffset = FVector(-20.0f, -20.0f, 12.5f);
    SpawnRotation = FVector(0.0f, 0.0f, 0.0f);
}

void ARobotActor::initialize()
{
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkeletalMeshAsset(*meshPath);
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

    if (SkeletalMeshComp)
    {
        // Enable collision
        SkeletalMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        SkeletalMeshComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
        // Enable hit notifications
        SkeletalMeshComp->SetNotifyRigidBodyCollision(true);
        SkeletalMeshComp->OnComponentHit.AddDynamic(this, &ARobotActor::OnSkeletalMeshHit);
    }
}

// Called when the game starts or when spawned
void ARobotActor::BeginPlay()
{
	Super::BeginPlay();
    
}

// Called every frame
void ARobotActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (!initialized) {
        return;
    }

    recordConstraints();
    recordTransforms();
}

void ARobotActor::recordConstraints()
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

void ARobotActor::recordTransforms()
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

void ARobotActor::OnSkeletalMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // Store the collision data
    CollisionDataList.Add(FCollisionData(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit));

    // Logging
    if (OtherActor)
    {
        UE_LOG(LogTemp, Log, TEXT("Collision stored: HitComponent=%s, OtherActor=%s, OtherComp=%s, Impulse=%s, HitLocation=%s"),
            *HitComponent->GetName(),
            *OtherActor->GetName(),
            OtherComp ? *OtherComp->GetName() : TEXT("None"),
            *NormalImpulse.ToString(),
            *Hit.Location.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Collision with null actor!"));
    }
}
