// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnvironmentObjectActor.generated.h"

UCLASS()
class GAMEANIMATIONSAMPLE_API AEnvironmentObjectActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnvironmentObjectActor();
	bool initialized;

	UStaticMesh* Mesh;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	TSharedPtr<FJsonObject> GetMesh();
	void MoveTo(FVector TargetLocation, FRotator TargetRotation);
};
