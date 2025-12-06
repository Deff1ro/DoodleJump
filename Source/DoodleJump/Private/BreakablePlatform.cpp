#include "BreakablePlatform.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DoodleCharacter.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"

ABreakablePlatform::ABreakablePlatform()
{
	PrimaryActorTick.bCanEverTick = false;

	PlatformMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlatformMesh"));
	RootComponent = PlatformMesh;
	PlatformMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PlatformMesh->SetNotifyRigidBodyCollision(true);
	PlatformMesh->SetSimulatePhysics(false);
	PlatformMesh->SetEnableGravity(false);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(PlatformMesh);
	CollisionBox->SetBoxExtent(FVector(60.0f, 60.0f, 15.0f));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BreakDelay = 0.1f;
	bIsBroken = false;
	BreakAnimation = nullptr;
	bUsePhysics = false;
}

void ABreakablePlatform::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("=== BreakablePlatform BeginPlay START ==="));
	UE_LOG(LogTemp, Warning, TEXT("PlatformMesh valid: %s"), PlatformMesh ? TEXT("YES") : TEXT("NO"));

	if (PlatformMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlatformMesh Collision Enabled: %d"), (int32)PlatformMesh->GetCollisionEnabled());
		UE_LOG(LogTemp, Warning, TEXT("PlatformMesh Profile Name: %s"), *PlatformMesh->GetCollisionProfileName().ToString());
		UE_LOG(LogTemp, Warning, TEXT("PlatformMesh Notify Rigid Body Collision: %s"), PlatformMesh->BodyInstance.bNotifyRigidBodyCollision ? TEXT("YES") : TEXT("NO"));

		PlatformMesh->OnComponentHit.AddDynamic(this, &ABreakablePlatform::OnPlatformHit);
		UE_LOG(LogTemp, Warning, TEXT("Hit event bound successfully"));
	}

	UE_LOG(LogTemp, Warning, TEXT("=== BreakablePlatform BeginPlay END ==="));
}

void ABreakablePlatform::OnPlatformHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("OnPlatformHit triggered! OtherActor: %s"), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));

	if (bIsBroken)
	{
		UE_LOG(LogTemp, Warning, TEXT("Platform already broken, ignoring hit"));
		return;
	}

	ADoodleCharacter* Character = Cast<ADoodleCharacter>(OtherActor);
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit actor is not DoodleCharacter, ignoring"));
		return;
	}

	FVector HitNormal = Hit.Normal;
	UE_LOG(LogTemp, Warning, TEXT("Hit Normal: %s (Z: %f)"), *HitNormal.ToString(), HitNormal.Z);

	if (HitNormal.Z > 0.7f || HitNormal.Z < -0.7f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Valid hit direction, breaking platform!"));
		BreakPlatform();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid hit direction, ignoring"));
	}
}

void ABreakablePlatform::BreakPlatform()
{
	bIsBroken = true;
	UE_LOG(LogTemp, Warning, TEXT("BreakPlatform called! Delay: %f"), BreakDelay);

	GetWorld()->GetTimerManager().SetTimer(BreakTimerHandle, [this]()
	{
		UE_LOG(LogTemp, Warning, TEXT("Break timer fired!"));

		if (BreakAnimation)
		{
			UE_LOG(LogTemp, Warning, TEXT("BreakAnimation assigned: %s"), *BreakAnimation->GetName());

			if (UAnimInstance* AnimInstance = PlatformMesh->GetAnimInstance())
			{
				UE_LOG(LogTemp, Warning, TEXT("AnimInstance found: %s"), *AnimInstance->GetClass()->GetName());
				UE_LOG(LogTemp, Warning, TEXT("Playing animation via AnimInstance"));
				AnimInstance->Montage_Play(Cast<UAnimMontage>(BreakAnimation), 1.0f);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("No AnimInstance, playing animation directly on mesh"));
				PlatformMesh->PlayAnimation(BreakAnimation, false);
			}

			float AnimDuration = BreakAnimation->GetPlayLength();
			UE_LOG(LogTemp, Warning, TEXT("Animation duration: %f seconds"), AnimDuration);

			GetWorld()->GetTimerManager().SetTimer(PhysicsTimerHandle, [this]()
			{
				UE_LOG(LogTemp, Warning, TEXT("Animation finished! Enabling gravity (bUsePhysics: %s)"), bUsePhysics ? TEXT("YES") : TEXT("NO"));

				if (bUsePhysics)
				{
					UE_LOG(LogTemp, Warning, TEXT("Enabling physics simulation"));
					PlatformMesh->SetSimulatePhysics(true);
					PlatformMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
					PlatformMesh->SetEnableGravity(true);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Enabling gravity only (no physics)"));
					// For gravity-only, we need physics simulation to apply gravity
					PlatformMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					PlatformMesh->SetSimulatePhysics(true);
					PlatformMesh->SetEnableGravity(true);
				}

				UE_LOG(LogTemp, Warning, TEXT("Gravity enabled: %s"), PlatformMesh->IsGravityEnabled() ? TEXT("YES") : TEXT("NO"));
				UE_LOG(LogTemp, Warning, TEXT("Simulating physics: %s"), PlatformMesh->IsSimulatingPhysics() ? TEXT("YES") : TEXT("NO"));
			}, AnimDuration, false);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No BreakAnimation assigned! Just enabling gravity"));

			if (bUsePhysics)
			{
				PlatformMesh->SetSimulatePhysics(true);
				PlatformMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
			}
			else
			{
				// For gravity-only, we need physics simulation to apply gravity
				PlatformMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				PlatformMesh->SetSimulatePhysics(true);
			}

			PlatformMesh->SetEnableGravity(true);

			UE_LOG(LogTemp, Warning, TEXT("Gravity enabled: %s"), PlatformMesh->IsGravityEnabled() ? TEXT("YES") : TEXT("NO"));
			UE_LOG(LogTemp, Warning, TEXT("Simulating physics: %s"), PlatformMesh->IsSimulatingPhysics() ? TEXT("YES") : TEXT("NO"));
		}
	}, BreakDelay, false);
}

