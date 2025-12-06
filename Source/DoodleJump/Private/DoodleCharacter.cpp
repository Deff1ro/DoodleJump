#include "DoodleCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TimerManager.h"

ADoodleCharacter::ADoodleCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetGenerateOverlapEvents(true);
	}

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	UCharacterMovementComponent* CharMovement = GetCharacterMovement();
	if (CharMovement)
	{
		CharMovement->MaxWalkSpeed = 600.0f;
		CharMovement->AirControl = 1.0f;
		CharMovement->GravityScale = 1.5f;
		CharMovement->JumpZVelocity = 600.0f;
		CharMovement->BrakingDecelerationWalking = 0.0f;
	}

	MovementSpeed = 600.0f;
	JumpForce = 600.0f;
	CustomGravityScale = 1.5f;
	CustomAirControl = 1.0f;
	MouseSensitivity = 1.0f;
	MinPitch = -80.0f;
	MaxPitch = 80.0f;
	SpringArmLength = 300.0f;
	RotationSpeed = 180.0f;
	JumpBoostMultiplier = 1.5f;
	DefaultFreezeDuration = 5.0f;
	bIsFrozen = false;
	FreezeAttachmentActor = nullptr;
}

void ADoodleCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}

	UCharacterMovementComponent* CharMovement = GetCharacterMovement();
	if (CharMovement)
	{
		CharMovement->MaxWalkSpeed = MovementSpeed;
		CharMovement->AirControl = CustomAirControl;
		CharMovement->GravityScale = CustomGravityScale;
		CharMovement->JumpZVelocity = JumpForce;
	}

	if (SpringArm)
	{
		SpringArm->TargetArmLength = SpringArmLength;
	}
}

void ADoodleCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If frozen and attached to moving object, follow it manually
	if (bIsFrozen && FreezeAttachmentActor)
	{
		FVector NewLocation = FreezeAttachmentActor->GetActorLocation() + FreezeRelativeOffset;
		// USE SWEEP TO PRESERVE COLLISIONS!
		SetActorLocation(NewLocation, true);  // bSweep = true preserves collision detection
	}

	AutoJump();
	AutoRotate(DeltaTime);
}

void ADoodleCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADoodleCharacter::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADoodleCharacter::Look);
		}
	}
}

void ADoodleCharacter::Move(const FInputActionValue& Value)
{
	if (bIsFrozen)
	{
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ADoodleCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X * MouseSensitivity);

		float NewPitch = GetControlRotation().Pitch + (LookAxisVector.Y * MouseSensitivity);
		NewPitch = FMath::ClampAngle(NewPitch, MinPitch, MaxPitch);

		FRotator NewRotation = GetControlRotation();
		NewRotation.Pitch = NewPitch;
		Controller->SetControlRotation(NewRotation);
	}
}

void ADoodleCharacter::AutoJump()
{
	if (bIsFrozen)
	{
		return;
	}

	if (CanJump())
	{
		Jump();
	}
}

void ADoodleCharacter::AutoRotate(float DeltaTime)
{
	if (bIsFrozen)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		FRotator NewRotation = MeshComp->GetRelativeRotation();
		NewRotation.Yaw += RotationSpeed * DeltaTime;
		MeshComp->SetRelativeRotation(NewRotation);
	}
}

void ADoodleCharacter::ActivateJumpBoost(float Multiplier)
{
	UCharacterMovementComponent* CharMovement = GetCharacterMovement();
	if (!CharMovement) return;

	UE_LOG(LogTemp, Warning, TEXT("==== JUMP BOOST ACTIVATED ===="));
	UE_LOG(LogTemp, Warning, TEXT("Multiplier: %.2f"), Multiplier);
	UE_LOG(LogTemp, Warning, TEXT("IsFalling: %s"), CharMovement->IsFalling() ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("Current Velocity Z: %.2f"), CharMovement->Velocity.Z);

	// Calculate boosted velocity
	float BaseJumpVelocity = CharMovement->JumpZVelocity;
	float BoostedVelocity = BaseJumpVelocity * Multiplier;

	// Apply immediate upward velocity (LaunchCharacter doesn't require being on ground)
	FVector LaunchVelocity = FVector(0.0f, 0.0f, BoostedVelocity);
	LaunchCharacter(LaunchVelocity, false, true);

	UE_LOG(LogTemp, Warning, TEXT("Launched character with velocity: %.2f (Base: %.2f)"), BoostedVelocity, BaseJumpVelocity);
	UE_LOG(LogTemp, Warning, TEXT("New Velocity Z: %.2f"), CharMovement->Velocity.Z);
}

void ADoodleCharacter::FreezeCharacter(float Duration, AActor* AttachToActor)
{
	UE_LOG(LogTemp, Warning, TEXT("==== CHARACTER FROZEN ===="));
	UE_LOG(LogTemp, Warning, TEXT("Freeze Duration: %.2f seconds"), Duration);
	UE_LOG(LogTemp, Warning, TEXT("Attach To Actor: %s"), AttachToActor ? *AttachToActor->GetName() : TEXT("None"));

	// ONLY set the freeze flag - nothing else!
	bIsFrozen = true;
	FreezeAttachmentActor = AttachToActor;

	// Store relative offset from attachment actor for manual position sync in Tick
	if (AttachToActor)
	{
		FreezeRelativeOffset = GetActorLocation() - AttachToActor->GetActorLocation();
		UE_LOG(LogTemp, Warning, TEXT("Stored relative offset: %s"), *FreezeRelativeOffset.ToString());
	}

	UE_LOG(LogTemp, Warning, TEXT("Freeze flag set - control and auto-jump DISABLED, collision ACTIVE"));

	// Set timer to unfreeze after duration
	GetWorld()->GetTimerManager().SetTimer(FreezeTimerHandle, this, &ADoodleCharacter::UnfreezeCharacter, Duration, false);
}

void ADoodleCharacter::UnfreezeCharacter()
{
	UCharacterMovementComponent* CharMovement = GetCharacterMovement();
	if (!CharMovement) return;

	UE_LOG(LogTemp, Error, TEXT("==== CHARACTER UNFROZEN - FUNCTION CALLED ===="));

	bIsFrozen = false;

	// Clear attachment reference
	if (FreezeAttachmentActor)
	{
		UE_LOG(LogTemp, Error, TEXT("Clearing reference to '%s'"), *FreezeAttachmentActor->GetName());
		FreezeAttachmentActor = nullptr;
	}

	// Immediately launch character upward (auto-jump after unfreeze)
	FVector JumpVelocity = FVector(0.0f, 0.0f, CharMovement->JumpZVelocity);
	UE_LOG(LogTemp, Error, TEXT("Launching character with velocity: %.2f"), CharMovement->JumpZVelocity);
	LaunchCharacter(JumpVelocity, false, true);

	UE_LOG(LogTemp, Error, TEXT("Character velocity Z after launch: %.2f"), CharMovement->Velocity.Z);
	UE_LOG(LogTemp, Error, TEXT("==== UNFREEZE COMPLETE ===="));
}

void ADoodleCharacter::ApplyKnockback(FVector Direction, float Force)
{
	UCharacterMovementComponent* CharMovement = GetCharacterMovement();
	if (!CharMovement) return;

	UE_LOG(LogTemp, Warning, TEXT("==== KNOCKBACK REQUESTED ===="));
	UE_LOG(LogTemp, Warning, TEXT("Is Frozen: %s"), bIsFrozen ? TEXT("YES") : TEXT("NO"));

	// If character is frozen, unfreeze them first
	if (bIsFrozen)
	{
		UE_LOG(LogTemp, Warning, TEXT("Character is frozen - unfreezing before applying knockback"));

		// Cancel freeze timer
		if (FreezeTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(FreezeTimerHandle);
			UE_LOG(LogTemp, Warning, TEXT("Freeze timer cleared"));
		}

		// Clear reference to trap/platform
		if (FreezeAttachmentActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Clearing reference to '%s'"), *FreezeAttachmentActor->GetName());
			FreezeAttachmentActor = nullptr;
		}

		// Clear frozen flag
		bIsFrozen = false;
		UE_LOG(LogTemp, Warning, TEXT("Character unfrozen successfully"));
	}

	// Normalize direction to ensure consistent knockback force
	FVector KnockbackDirection = Direction.GetSafeNormal();

	// Apply knockback impulse using LaunchCharacter
	FVector KnockbackVelocity = KnockbackDirection * Force;
	LaunchCharacter(KnockbackVelocity, true, true);

	UE_LOG(LogTemp, Warning, TEXT("Knockback applied! Direction: %s, Force: %.2f"), *KnockbackDirection.ToString(), Force);
	UE_LOG(LogTemp, Warning, TEXT("Character velocity after knockback: %s"), *CharMovement->Velocity.ToString());
}

