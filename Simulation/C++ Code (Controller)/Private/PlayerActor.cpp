// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerActor.h"

// Sets default values
APlayerActor::APlayerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    isDone = false;
}

// Called when the game starts or when spawned
void APlayerActor::BeginPlay()
{
	Super::BeginPlay();

    //TransformBones.Add(TEXT("pelvis"));

    
    TransformBones = {
        TEXT("pelvis"), TEXT("spine_02"), TEXT("spine_03"), TEXT("spine_04"), TEXT("spine_05"),
        TEXT("neck_01"), TEXT("neck_02"), TEXT("head"),
        TEXT("clavicle_l"), TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"),
        TEXT("clavicle_r"), TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"),
        TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), TEXT("ball_l"),
        TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), TEXT("ball_r")
    };

    initialized = false;
}


// Called every frame
void APlayerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


    //TMap<FString, FTransform> transforms = recordTransforms();

    // Check if the array is not empty
    if (CollisionDataList.Num() > 0)
    {
        // Get the first item
        FCollisionData& FirstItem = CollisionDataList[0];

        // Debug message to print the first item
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("CollisionData found: %s"), *FirstItem.ToString()));

        // Remove the first item from the array
        CollisionDataList.RemoveAt(0);
    }
    else
    {
        //GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("No collision data available to remove."));
    }


}

void APlayerActor::Respawn(FVector SpawnLocation, FRotator SpawnRotation)
{
    isDone = false;
    initialized = true;

    FTransform OriginTransform(FQuat(SpawnRotation), SpawnLocation);
    OriginInverse = OriginTransform.Inverse();

    //OriginTransform = FTransform(SpawnLocation, SpawnRotation);

    SkeletalMeshComp->SetCollisionProfileName(TEXT("BlockAll"));
    SkeletalMeshComp->SetNotifyRigidBodyCollision(true);
    SkeletalMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    SkeletalMeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Block);
}

TSharedPtr<FJsonObject> APlayerActor::getState()
{
    TMap<FString, FTransform> TransformMap = recordTransforms();

    // Create the root JSON object
    TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();

    // Create the "Transforms" object
    TSharedPtr<FJsonObject> TransformsObject = MakeShared<FJsonObject>();

    // Iterate through the TMap and add each transform as a JSON object
    for (const auto& Pair : TransformMap)
    {
        const FString& Key = Pair.Key;
        const FTransform& Transform = Pair.Value;

        // Create an object for this key
        TSharedPtr<FJsonObject> TransformObject = MakeShared<FJsonObject>();

        // Serialize the Translation
        TSharedPtr<FJsonObject> TranslationObject = MakeShared<FJsonObject>();
        TranslationObject->SetNumberField("X", Transform.GetTranslation().X);
        TranslationObject->SetNumberField("Y", Transform.GetTranslation().Y);
        TranslationObject->SetNumberField("Z", Transform.GetTranslation().Z);

        // Serialize the Rotation
        TSharedPtr<FJsonObject> RotationObject = MakeShared<FJsonObject>();
        const FQuat Rotation = Transform.GetRotation();
        RotationObject->SetNumberField("X", Rotation.X);
        RotationObject->SetNumberField("Y", Rotation.Y);
        RotationObject->SetNumberField("Z", Rotation.Z);
        RotationObject->SetNumberField("W", Rotation.W);

        // Add Translation and Rotation to the Transform object
        TransformObject->SetObjectField("Translation", TranslationObject);
        TransformObject->SetObjectField("Rotation", RotationObject);

        // Add the Transform object to the "Transforms" object with the key
        TransformsObject->SetObjectField(Key, TransformObject);
    }

    // Add "Transforms" to the root object
    RootObject->SetObjectField("Transforms", TransformsObject);

    return RootObject;
}

TMap<FString, FTransform> APlayerActor::recordTransforms()
{
    
    TMap<FString, FTransform> TransformMap;

    if (SkeletalMeshComp) {
        // Iterate over the TArray<FString>
        for (const FString& BoneNameString : TransformBones) {
            // Convert FString to FName
            FName BoneName(*BoneNameString);

            // Check if the bone exists
            if (SkeletalMeshComp->DoesSocketExist(BoneName)) {
                // Get the world transform of the bone
                FTransform BoneWorldTransform = SkeletalMeshComp->GetSocketTransform(BoneName, RTS_World);
                FTransform BoneRelativeTransform = BoneWorldTransform * OriginInverse;

                // Add the result to TransformMap
                TransformMap.Add(BoneNameString, BoneRelativeTransform);

                // Debug: Print the transform to the screen
                //FString TransformString = BoneWorldTransform.ToString();
                //GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Bone '%s' World Transform: %s"), *BoneName.ToString(), *TransformString));
            } else {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Bone '%s' does not exist in the skeletal mesh!"), *BoneName.ToString()));
            }
        }
    } else {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("SkeletalMeshComp is null."));
    }

    return TransformMap;

}

void APlayerActor::OnFloorHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    //by default ignore
}
