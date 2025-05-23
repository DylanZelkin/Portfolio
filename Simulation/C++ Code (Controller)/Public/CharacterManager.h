// CharacterManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerActor.h"
#include "CharacterManager.generated.h"

UCLASS()
class GAMEANIMATIONSAMPLE_API ACharacterManager : public APlayerActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACharacterManager();

	virtual void Respawn(FVector SpawnLocation, FRotator SpawnRotation) override;

	virtual void OnBarrierHit(UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Reference to the spawned character
	UPROPERTY()
	class ACharacter* ManagedCharacter;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FString move_state;
	void mapActionToControls(TMap<FString, FString> KeyControlsMap);

	virtual void setAction(TSharedPtr<FJsonObject> action) override;



};
