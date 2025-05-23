// Fill out your copyright notice in the Description page of Project Settings.

#include "RobotActorManager.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"


// Sets default values
ARobotActorManager::ARobotActorManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    FloorInvulnerableBodies = { "foot_r", "ball_r", "foot_l", "ball_l"/*, "calf_l", "calf_r"*/};
}

void ARobotActorManager::Respawn(FVector SpawnLocation, FRotator SpawnRotation)
{
    FRotator SpawnRotationRobot = SpawnRotation;
    SpawnRotationRobot.Yaw -= 90.0f;

    //delete existing one if there is one
    if (SkeletalMeshComp) {
        SkeletalMeshComp->SetSimulatePhysics(false);
        SkeletalMeshComp->BodyInstance.SetUseCCD(false);

        SkeletalMeshComp->RefreshBoneTransforms();
        SkeletalMeshComp->ResetAllBodiesSimulatePhysics();
        SkeletalMeshComp->RecreatePhysicsState();

        SkeletalMeshComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
        SkeletalMeshComp->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        //SkeletalMeshComp->setforce
        for (FBodyInstance* BodyInst : SkeletalMeshComp->Bodies) {
            if (BodyInst) {
                BodyInst->ClearForces();
                BodyInst->ClearTorques();
                BodyInst->ClearExternalCollisionProfile();
                BodyInst->SetLinearVelocity(FVector::ZeroVector, false);
                BodyInst->SetAngularVelocityInRadians(FVector::ZeroVector, false);
            }
        }
        SkeletalMeshComp->BodyInstance.SetUseCCD(true);
        
    } else {
        UPhysicsAsset* PhysicsAsset = LoadObject<UPhysicsAsset>(nullptr, TEXT("/Game/Characters/UE5_Mannequins/Rigs/PA_Mannequin_Adjusted.PA_Mannequin_Adjusted"));

        if (PhysicsAsset) {
            // Retrieve the skeletal mesh it was created for
            USkeletalMesh* AssociatedSkeletalMesh = PhysicsAsset->PreviewSkeletalMesh.LoadSynchronous();
            if (AssociatedSkeletalMesh) {
                SkeletalMeshComp = NewObject<USkeletalMeshComponent>(this);

                // Set the skeletal mesh to the component.
                if (SkeletalMeshComp) {
                    SkeletalMeshComp->SetSkeletalMesh(AssociatedSkeletalMesh);
                    SkeletalMeshComp->SetPhysicsAsset(PhysicsAsset);

                    SkeletalMeshComp->SetupAttachment(RootComponent);
                    SkeletalMeshComp->RegisterComponent();

                    SkeletalMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                    SkeletalMeshComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

                    SkeletalMeshComp->SetNotifyRigidBodyCollision(true);

                    //SkeletalMeshComp->SetSimulatePhysics(true);
                    SkeletalMeshComp->BodyInstance.SetUseCCD(true);
                } else {
                    GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Problem with SkeletalMeshComp in RobotActorManager");
                    return;
                }
            } else {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "No associated Skeletal Mesh found in Physics Asset.");
                return;
            }
        } else {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to load Physics Asset.");
            return;
        }
    }
    //FVector AdjustedSpawnLocation = SpawnLocation;
    //AdjustedSpawnLocation.Z += 0.5; //add small buffer, so it doesnt get stuck sometimes
    SkeletalMeshComp->SetWorldLocationAndRotation(SpawnLocation, SpawnRotationRobot, false, nullptr, ETeleportType::ResetPhysics);

    firstAction = true;
    Super::Respawn(SpawnLocation, SpawnRotation);
}

// Called when the game starts or when spawned
void ARobotActorManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ARobotActorManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TMap<FString, FVector> ARobotActorManager::recordConstraints()
{
	TMap<FString, FVector> ConstraintMap;

	UPhysicsAsset* PhysicsAsset = SkeletalMeshComp->GetPhysicsAsset();
	if (!PhysicsAsset) {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Physics Asset not found! 1");
    } else {
        for (UPhysicsConstraintTemplate* ConstraintTemplate : PhysicsAsset->ConstraintSetup) {
            if (!ConstraintTemplate) {
                continue;
            }
            FConstraintInstance* ConstraintInstance = SkeletalMeshComp->FindConstraintInstance(ConstraintTemplate->DefaultInstance.JointName);
            if (!ConstraintInstance) {
                continue;
            }
            float AngleSwing1 = ConstraintInstance->GetCurrentSwing1();
            float AngleSwing2 = ConstraintInstance->GetCurrentSwing2();
            float AngleTwist = ConstraintInstance->GetCurrentTwist();

            ConstraintMap.Add(ConstraintTemplate->DefaultInstance.JointName.ToString(), FVector(AngleSwing1, AngleSwing2, AngleTwist));
        }
    }
	return ConstraintMap;
}

void ARobotActorManager::updateConstraints(TMap<FString, FVector> motor_controls)
{
    UPhysicsAsset* PhysicsAsset = SkeletalMeshComp->GetPhysicsAsset();
    if (!PhysicsAsset) {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Physics Asset not found! 2");
        return;
    } else {
        for (UPhysicsConstraintTemplate* ConstraintTemplate : PhysicsAsset->ConstraintSetup) {
            if (!ConstraintTemplate) {
                continue;
            }
            if (FVector* TargetAngularVelocity = motor_controls.Find(ConstraintTemplate->DefaultInstance.JointName.ToString())) {
                FConstraintInstance* ConstraintInstance = SkeletalMeshComp->FindConstraintInstance(ConstraintTemplate->DefaultInstance.JointName);
                if (!ConstraintInstance) {
                    continue;
                };
                ConstraintInstance->SetAngularVelocityTarget(*TargetAngularVelocity);

                /*FString TargetAngularVelocityStr = TargetAngularVelocity->ToString();
                FString DebugMessage = FString::Printf(TEXT("SET VELOCITY: %s"), *TargetAngularVelocityStr);
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, DebugMessage);*/

               // ConstraintInstance->SetAngularDrivePa
            } else {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "TargetAngularVelocity failure");
            }
        }
    }
}

TSharedPtr<FJsonObject> ARobotActorManager::getState()
{
    // Get the constraints map
    TMap<FString, FVector> ConstraintsMap = recordConstraints();

    // Create the root JSON object for constraints
    TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();

    // Create the "Constraints" object
    TSharedPtr<FJsonObject> ConstraintsObject = MakeShared<FJsonObject>();

    // Iterate through the ConstraintsMap and add each constraint as a JSON object
    for (const auto& Pair : ConstraintsMap)
    {
        const FString& Key = Pair.Key;
        const FVector& Constraint = Pair.Value;

        // Create an object for this key (constraint)
        TSharedPtr<FJsonObject> ConstraintObject = MakeShared<FJsonObject>();

        // Serialize the constraint components (AngleSwing1, AngleSwing2, AngleTwist)
        ConstraintObject->SetNumberField("Swing1", Constraint.X);
        ConstraintObject->SetNumberField("Swing2", Constraint.Y);
        ConstraintObject->SetNumberField("Twist", Constraint.Z);

        // Add the constraint object to the "Constraints" object with the key
        ConstraintsObject->SetObjectField(Key, ConstraintObject);
    }

    // Add "Constraints" to the root object
    RootObject->SetObjectField("Constraints", ConstraintsObject);

    // Get the parent JSON object from APlayerActor
    TSharedPtr<FJsonObject> ParentJson = APlayerActor::getState();

    // Combine the parent JSON and the current JSON into one root object
    // Add the "Transforms" from the parent JSON to this root object
    RootObject->SetObjectField("Transforms", ParentJson->GetObjectField("Transforms"));

    return RootObject;
}

void ARobotActorManager::setAction(TSharedPtr<FJsonObject> JsonObject)
{
    TMap<FString, FVector> ConstraintPowersMap;

    if (firstAction) {
        SkeletalMeshComp->SetSimulatePhysics(true);
        firstAction = false;
    }

    if (!JsonObject.IsValid()) {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Invalid JSON object in RobotActor setAction");
        return;
    }

    // Get the "ConstraintPowers" object from the JSON
    const TSharedPtr<FJsonObject>* ConstraintPowersObject;
    if (JsonObject->TryGetObjectField(TEXT("ConstraintPowers"), ConstraintPowersObject)) {
        // Iterate through each constraint name and its associated power controls
        for (const auto& Pair : (*ConstraintPowersObject)->Values) {
            const FString& ConstraintName = Pair.Key;
            const TSharedPtr<FJsonObject>* ConstraintObject;
            //(*ConstraintObject)->Get
            //Pair.Value->AsNumber()
            // Ensure the value is a JSON object
            if (Pair.Value->TryGetObject(ConstraintObject)) {
                // Extract Swing1, Swing2, and Twist values
                double Swing1 = (*ConstraintObject)->GetNumberField(TEXT("Swing1"));
                double Swing2 = (*ConstraintObject)->GetNumberField(TEXT("Swing2"));
                double Twist = (*ConstraintObject)->GetNumberField(TEXT("Twist"));

                /*FString DebugMessage = FString::Printf(TEXT("Constraint: %s, Swing1: %.2f, Swing2: %.2f, Twist: %.2f"), *ConstraintName, Swing1, Swing2, Twist);
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, DebugMessage);*/

                // Add to the TMap
                ConstraintPowersMap.Add(ConstraintName, FVector(Swing1, Swing2, Twist));
            }
        }
    } else {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to find 'ConstraintPowers' header in JSON object in RobotActor setAction");
        return;
    }

    updateConstraints(ConstraintPowersMap);

}

void ARobotActorManager::OnFloorHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherComp && OtherComp == SkeletalMeshComp) {
        //GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("Hit MyReference SkeletalMeshComponent!"));
        FName BodyName = Hit.BoneName;

        if (FloorInvulnerableBodies.Contains(BodyName.ToString())) {
            //GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Collision with invulnerable body");
        } else {
            isDone = true;
            /*GEngine->AddOnScreenDebugMessage(
                -1, 5.0f, FColor::Yellow,
                FString::Printf(TEXT("Hit Body Name: %s"), *BodyName.ToString())
            );*/
        }
    }
}

void ARobotActorManager::OnBarrierHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherComp && OtherComp == SkeletalMeshComp) {
        isDone = true;
        //GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Robot hit the barrier!"));
    }
}