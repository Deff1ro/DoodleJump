#include "MovingPlatform.h"
#include "Components/StaticMeshComponent.h"
#include "MovementPoint.h"

AMovingPlatform::AMovingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	RootComponent = PlatformMesh;
	PlatformMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PlatformMesh->SetSimulatePhysics(false);

	Speed = 200.0f;
	bLoopMovement = true;
	CurrentPointIndex = 0;
	bMovingForward = true;
}

void AMovingPlatform::BeginPlay()
{
	Super::BeginPlay();

	if (MovementPoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("MovingPlatform '%s': No movement points assigned!"), *GetName());
		SetActorTickEnabled(false);
		return;
	}

	// FIRST: Attach all objects BEFORE teleporting the platform
	// This way they will teleport together with the platform
	for (AActor* AttachedObject : AttachedObjects)
	{
		if (AttachedObject)
		{
			AttachedObject->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			UE_LOG(LogTemp, Log, TEXT("Attached '%s' to MovingPlatform '%s'"), *AttachedObject->GetName(), *GetName());
		}
	}

	// THEN: Teleport platform to first point (attached objects will move with it)
	if (MovementPoints[0])
	{
		SetActorLocation(MovementPoints[0]->GetActorLocation());
	}

	// Start moving towards the second point (index 1)
	CurrentPointIndex = 1;

	UE_LOG(LogTemp, Log, TEXT("MovingPlatform '%s' initialized with %d points, Speed: %.2f, Loop: %s, Attached Objects: %d"),
		*GetName(), MovementPoints.Num(), Speed, bLoopMovement ? TEXT("YES") : TEXT("NO"), AttachedObjects.Num());
}

void AMovingPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MovementPoints.Num() < 2)
	{
		return;
	}

	MoveTowardsTarget(DeltaTime);
}

void AMovingPlatform::MoveTowardsTarget(float DeltaTime)
{
	// Validate current target point
	if (!MovementPoints.IsValidIndex(CurrentPointIndex) || !MovementPoints[CurrentPointIndex])
	{
		UE_LOG(LogTemp, Warning, TEXT("MovingPlatform '%s': Invalid movement point at index %d"), *GetName(), CurrentPointIndex);
		return;
	}

	FVector CurrentLocation = GetActorLocation();
	FVector TargetLocation = MovementPoints[CurrentPointIndex]->GetActorLocation();
	FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
	float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);

	float StepDistance = Speed * DeltaTime;

	// Check if we reached the target
	if (DistanceToTarget <= StepDistance)
	{
		// Snap to target
		SetActorLocation(TargetLocation);

		// Move to next point
		if (bMovingForward)
		{
			CurrentPointIndex++;
			if (CurrentPointIndex >= MovementPoints.Num())
			{
				if (bLoopMovement)
				{
					// Loop back to start
					CurrentPointIndex = 0;
				}
				else
				{
					// Reverse direction
					CurrentPointIndex = MovementPoints.Num() - 2;
					bMovingForward = false;
				}
			}
		}
		else
		{
			CurrentPointIndex--;
			if (CurrentPointIndex < 0)
			{
				// Reverse direction
				CurrentPointIndex = 1;
				bMovingForward = true;
			}
		}
	}
	else
	{
		// Move towards target
		FVector NewLocation = CurrentLocation + (Direction * StepDistance);
		SetActorLocation(NewLocation);
	}
}

