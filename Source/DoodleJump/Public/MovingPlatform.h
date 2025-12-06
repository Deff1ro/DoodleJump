// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovingPlatform.generated.h"

class UStaticMeshComponent;
class AMovementPoint;

UCLASS()
class DOODLEJUMP_API AMovingPlatform : public AActor
{
	GENERATED_BODY()

public:
	AMovingPlatform();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PlatformMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	TArray<AMovementPoint*> MovementPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	bool bLoopMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	TArray<AActor*> AttachedObjects;

private:
	int32 CurrentPointIndex;
	bool bMovingForward;

	void MoveTowardsTarget(float DeltaTime);
};
