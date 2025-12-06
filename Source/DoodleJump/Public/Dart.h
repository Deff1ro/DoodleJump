// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dart.generated.h"

class UStaticMeshComponent;
class UCapsuleComponent;

UCLASS()
class DOODLEJUMP_API ADart : public AActor
{
	GENERATED_BODY()

public:
	ADart();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DartMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CollisionCapsule;

	// Movement parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float DartSpeed;

	// Knockback parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float KnockbackForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float DotProductThreshold;

	// Lifetime
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float Lifetime;

private:
	UFUNCTION()
	void OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
