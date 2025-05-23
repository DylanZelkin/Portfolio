// CharacterManager.cpp
#include "CharacterManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"

// Sets default values
ACharacterManager::ACharacterManager()
{
	// Enable ticking
	PrimaryActorTick.bCanEverTick = true;

	// Set the character reference to null
	ManagedCharacter = nullptr;
}

// Called when the game starts or when spawned
void ACharacterManager::BeginPlay()
{
	Super::BeginPlay();
}


void ACharacterManager::Respawn(FVector SpawnLocation, FRotator SpawnRotation)
{
    move_state = "Jog"; //MUST BE IN JOG BY DEFAULT

    //delete the character if it exists
    if (ManagedCharacter) {
        
        TMap<FString, FString> KeyControlsMap;
        KeyControlsMap.Add("Move", "None");
        KeyControlsMap.Add("State", "Jog");
        mapActionToControls(KeyControlsMap);

        ManagedCharacter->GetCharacterMovement()->StopActiveMovement();
        ManagedCharacter->GetCharacterMovement()->StopMovementImmediately();

        float CapsuleHalfHeight = ManagedCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        FVector AdjustedSpawnLocation = SpawnLocation;
        AdjustedSpawnLocation.Z += CapsuleHalfHeight;
        ManagedCharacter->SetActorLocationAndRotation(AdjustedSpawnLocation, SpawnRotation, false, nullptr, ETeleportType::ResetPhysics);
       
        UClass* OriginalAnimBP = SkeletalMeshComp->AnimClass;
        //SkeletalMeshComp->SetAnimInstanceClass(nullptr);
        //SkeletalMeshComp->SetAnimInstanceClass(OriginalAnimBP);
        
    } else {

        FString BlueprintPath = TEXT("/Game/Blueprints/RetargetedCharacters/CBP_SandboxCharacter_Manny.CBP_SandboxCharacter_Manny_C");

        // Dynamically load the blueprint class
        UClass* BlueprintClass = LoadObject<UClass>(nullptr, *BlueprintPath);
        if (!BlueprintClass) {
            FString ErrorMessage = FString::Printf(TEXT("Failed to load Blueprint at path: %s"), *BlueprintPath);
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, ErrorMessage);
            return;
        }

        // Define spawn parameters
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        // Spawn the character
        ManagedCharacter = GetWorld()->SpawnActor<ACharacter>(BlueprintClass, SpawnLocation, SpawnRotation, SpawnParams);
        if (ManagedCharacter) {
            // Ensure the character has gravity and proper collision
            UCharacterMovementComponent* MovementComponent = ManagedCharacter->GetCharacterMovement();
            if (MovementComponent) {
                MovementComponent->SetMovementMode(EMovementMode::MOVE_Walking); // Ensure walking mode
                MovementComponent->GravityScale = 1.0f; // Enable gravity
                MovementComponent->bOrientRotationToMovement = true; // Orient character to movement
            } else {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "MovementComponent fail");
                return;
            }

            // Enable collisions for the capsule component
            UCapsuleComponent* CapsuleComponent = ManagedCharacter->GetCapsuleComponent();
            if (CapsuleComponent) {
                CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                CapsuleComponent->SetSimulatePhysics(false); // Ensure physics simulation is off for walking characters
            } else {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "CapsuleComponent fail");
                return;
            }

            // Make sure the character is initialized properlyz
            ManagedCharacter->SetActorEnableCollision(true); // Ensure the actor has collision enabled
            //ManagedCharacter->FinishSpawning(FTransform(SpawnRotation, SpawnLocation)); // Finalize spawning process
            //ManagedCharacter->SetActorLocation(SpawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
            //NEEDED FOR PlayerActor.h
            SkeletalMeshComp = ManagedCharacter->GetMesh();

            ULocalPlayer* NewLocalPlayer = NewObject<ULocalPlayer>(GetWorld()->GetGameInstance(), ULocalPlayer::StaticClass());

            APlayerController* PlayerController = GetWorld()->SpawnActor<APlayerController>(APlayerController::StaticClass());

            PlayerController->SetPlayer(NewLocalPlayer);

            FPlatformUserId UserId;
            GetWorld()->GetGameInstance()->AddLocalPlayer(NewLocalPlayer, UserId);

            if (PlayerController) {
                PlayerController->Possess(ManagedCharacter);

                if (PlayerController == ManagedCharacter->GetController()) {
                    UInputMappingContext* InputMappingContext = LoadObject<UInputMappingContext>(
                        nullptr, TEXT("/Game/Input/IMC_Sandbox.IMC_Sandbox"));

                    if (InputMappingContext) {
                        UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

                        if (InputSubsystem) {
                            InputSubsystem->AddMappingContext(InputMappingContext, 0); // 0 is priority
                        } else {
                            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "InputSubsystem fail");
                        }
                    } else {
                        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "IMC fail");
                    }
                } else {
                    GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Controller fail");
                }
            } else {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Controller DNE");
            }
        } else {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to spawn character!");
        }
    }

    
    Super::Respawn(SpawnLocation, SpawnRotation);
}


// Called every frame
void ACharacterManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACharacterManager::mapActionToControls(TMap<FString, FString> KeyControlsMap)
{
    APlayerController* PlayerController = Cast<APlayerController>(ManagedCharacter->GetController());

    FString control = "Move";
    if (KeyControlsMap.Contains(control)) {
        FString FoundValue = KeyControlsMap[control];
        if (FoundValue == "None") {
            PlayerController->InputKey(EKeys::W, EInputEvent::IE_Released, 1.0f, false);
            PlayerController->InputKey(EKeys::A, EInputEvent::IE_Released, 1.0f, false);
            PlayerController->InputKey(EKeys::S, EInputEvent::IE_Released, 1.0f, false);
            PlayerController->InputKey(EKeys::D, EInputEvent::IE_Released, 1.0f, false);
        } else if (FoundValue == "Forward") {
            PlayerController->InputKey(EKeys::W, EInputEvent::IE_Pressed, 1.0f, false);
            PlayerController->InputKey(EKeys::A, EInputEvent::IE_Released, 1.0f, false);
            PlayerController->InputKey(EKeys::S, EInputEvent::IE_Released, 1.0f, false);
            PlayerController->InputKey(EKeys::D, EInputEvent::IE_Released, 1.0f, false);
        } else {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Could not match Move key");
        }
    } else {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Could not match Move control");
    }

    control = "State";
    if (KeyControlsMap.Contains(control)) {
        FString FoundValue = KeyControlsMap[control];
        bool change_off_walk = false;
        bool change_off_sprint = false;
        
        //if already in a state then just skip this: (...  && move_state != "...")
        if (FoundValue == "Walk" && move_state != "Walk") {
            if (move_state == "Sprint") change_off_sprint = true;
            change_off_walk = true;
            move_state = "Walk";
        } else if (FoundValue == "Jog" && move_state != "Jog") {
            if (move_state == "Walk") change_off_walk = true;
            else if (move_state == "Sprint") change_off_sprint = true;
            move_state = "Jog";
        } else if (FoundValue == "Sprint" && move_state != "Sprint") {
            if (move_state == "Walk") change_off_walk = true;
            PlayerController->InputKey(EKeys::LeftShift, EInputEvent::IE_Pressed, 1.0f, false);
            move_state = "Sprint";
        } else {
            //this isnt actually the error here ignore...
            //GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Could not match State key");
        }

        if (change_off_walk) {
            PlayerController->InputKey(EKeys::LeftControl, EInputEvent::IE_Pressed, 1.0f, false);
            GetWorld()->GetTimerManager().SetTimerForNextTick([PlayerController]() {
                    PlayerController->InputKey(EKeys::LeftControl, EInputEvent::IE_Released, 1.0f, false);});
        } else if (change_off_sprint) {
            PlayerController->InputKey(EKeys::LeftShift, EInputEvent::IE_Released, 1.0f, false);
        }
    } else {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Could not find State control");
    }
}

void ACharacterManager::setAction(TSharedPtr<FJsonObject> JsonObject)
{
    TMap<FString, FString> KeyControlsMap;

    if (!JsonObject.IsValid()) {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Invalid JSON object in CharacterManager setAction");
        return;
    }

    // Get the "KeyControls" object from the JSON
    const TSharedPtr<FJsonObject>* KeyControlsObject;
    if (JsonObject->TryGetObjectField(TEXT("KeyControls"), KeyControlsObject)) {
        // Iterate through each key-value pair in "KeyControls"
        for (const auto& Pair : (*KeyControlsObject)->Values) {
            const FString& Key = Pair.Key;
            FString Value;

            // Ensure the value is a string
            if (Pair.Value->TryGetString(Value)) {
                KeyControlsMap.Add(Key, Value); // Add the key-value pair to the map
            }
            else {
                FString WarningMessage = FString::Printf(TEXT("Key %s does not have a valid string value."), *Key);
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, WarningMessage);
            }
        }
    }
    else {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Failed to find 'KeyControls' header in JSON object in RobotActor setAction");
    }

    mapActionToControls(KeyControlsMap);

}

void ACharacterManager::OnBarrierHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    UCapsuleComponent* CapsuleComponent = ManagedCharacter->GetCapsuleComponent();

    if (CapsuleComponent && OtherComp == CapsuleComponent) {
        isDone = true;
        //GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Capsule component of ManagedCharacter hit the barrier!"));
    } else {
        //GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Collision with something other than the capsule!"));
    }
    
}