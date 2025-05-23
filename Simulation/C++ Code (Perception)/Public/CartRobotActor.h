// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RobotActor.h"
#include "CartRobotActor.generated.h"

/**
 * 
 */
UCLASS()
class THESISSIMULATOR_API ACartRobotActor : public ARobotActor
{
	GENERATED_BODY()

public:

	ACartRobotActor();

	virtual void initialize();
	/*ACartRobotActor();*/

	virtual void getState();
	virtual void setAction();

private:
};
