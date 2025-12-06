// Fill out your copyright notice in the Description page of Project Settings.

#include "Dart.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "DoodleCharacter.h"

ADart::ADart()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create Static Mesh Component (child of root)
	DartMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DartMesh"));
	DartMesh->SetupAttachment(RootComponent);

	// Disable collision on mesh completely
	DartMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create Capsule Component (child of root)
	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CollisionCapsule->SetupAttachment(RootComponent);
	CollisionCapsule->SetCapsuleHalfHeight(50.0f);
	CollisionCapsule->SetCapsuleRadius(10.0f);

	// Enable collision and simulation physics on capsule
	CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionCapsule->SetNotifyRigidBodyCollision(true); // Enable hit events

	// Default values
	DartSpeed = 500.0f;
	KnockbackForce = 1000.0f;
	DotProductThreshold = 0.5f;
	Lifetime = 10.0f; // 10 seconds by default
}

void ADart::BeginPlay()
{
	Super::BeginPlay();

	// Bind hit event
	if (CollisionCapsule)
	{
		CollisionCapsule->OnComponentHit.AddDynamic(this, &ADart::OnCapsuleHit);
		UE_LOG(LogTemp, Log, TEXT("Dart '%s' initialized. Speed: %.2f, Knockback Force: %.2f, Lifetime: %.2f seconds"), *GetName(), DartSpeed, KnockbackForce, Lifetime);
	}

	// Set lifetime for the dart - it will auto-destroy after this time
	SetLifeSpan(Lifetime);
}

void ADart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Move dart in local Y axis direction (right vector corresponds to Y axis)
	FVector LocalYDirection = GetActorRightVector();
	FVector CurrentLocation = GetActorLocation();
	FVector NewLocation = CurrentLocation + (LocalYDirection * DartSpeed * DeltaTime);
	SetActorLocation(NewLocation);
}

void ADart::OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Check if we hit the player
	ADoodleCharacter* HitCharacter = Cast<ADoodleCharacter>(OtherActor);
	if (!HitCharacter)
	{
		return; // Not the player, ignore
	}

	// Get dart velocity direction (using actor's local Y axis = right vector)
	FVector DartVelocity = GetActorRightVector();

	// Calculate direction from dart to player
	FVector DartLocation = GetActorLocation();
	FVector PlayerLocation = HitCharacter->GetActorLocation();
	FVector ToPlayerDirection = (PlayerLocation - DartLocation).GetSafeNormal();

	// Calculate dot product to check if dart was flying TOWARDS player
	float DotProduct = FVector::DotProduct(DartVelocity, ToPlayerDirection);

	UE_LOG(LogTemp, Warning, TEXT("==== DART HIT PLAYER ===="));
	UE_LOG(LogTemp, Warning, TEXT("Dart Velocity Direction: %s"), *DartVelocity.ToString());
	UE_LOG(LogTemp, Warning, TEXT("Direction To Player: %s"), *ToPlayerDirection.ToString());
	UE_LOG(LogTemp, Warning, TEXT("Dot Product: %.3f (Threshold: %.3f)"), DotProduct, DotProductThreshold);

	// If dot product > threshold, dart was flying INTO player -> apply knockback
	if (DotProduct > DotProductThreshold)
	{
		UE_LOG(LogTemp, Warning, TEXT(">>> DART HIT PLAYER - APPLYING KNOCKBACK! <<<"));

		// Apply knockback in the direction of dart's movement
		HitCharacter->ApplyKnockback(DartVelocity, KnockbackForce);

		// Destroy the dart
		Destroy();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT(">>> PLAYER LANDED ON DART - NO KNOCKBACK (acts as platform) <<<"));
		// Player landed on dart or dart hit from wrong angle - do nothing, collision works as normal platform
	}
}