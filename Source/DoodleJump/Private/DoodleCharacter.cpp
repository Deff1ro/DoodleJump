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

		// COMPLETELY remove inertia - instant stop when input released
		CharMovement->BrakingDecelerationWalking = 10000.0f;  // Extremely high for instant stop
		CharMovement->GroundFriction = 100.0f;  // Maximum friction
		CharMovement->BrakingFrictionFactor = 10.0f;  // Maximum friction when braking
		CharMovement->bUseSeparateBrakingFriction = true;  // Enable separate braking friction
		CharMovement->MaxAcceleration = 10000.0f;  // Instant acceleration

		// CRITICAL: Disable automatic velocity inheritance from moving platforms
		CharMovement->bImpartBaseVelocityX = false;  // Don't inherit X velocity from base
		CharMovement->bImpartBaseVelocityY = false;  // Don't inherit Y velocity from base
		CharMovement->bImpartBaseVelocityZ = false;  // Don't inherit Z velocity from base
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
	bIsKnockedBack = false;
	CurrentMovementInput = FVector2D::ZeroVector;
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

	UCharacterMovementComponent* CharMovement = GetCharacterMovement();

	// CRITICAL: Force clear horizontal velocity EVERY frame to prevent ANY external forces
	// Only vertical velocity (Z) is preserved for jumping/gravity
	// EXCEPTION: Don't clear during knockback - let LaunchCharacter velocity work
	if (CharMovement && !bIsKnockedBack)
	{
		FVector CurrentVelocity = CharMovement->Velocity;
		CharMovement->Velocity = FVector(0.0f, 0.0f, CurrentVelocity.Z);  // Keep only Z (vertical) velocity
	}

	// If frozen and attached to moving object, follow it manually
	if (bIsFrozen && FreezeAttachmentActor)
	{
		FVector NewLocation = FreezeAttachmentActor->GetActorLocation() + FreezeRelativeOffset;
		// USE SWEEP TO PRESERVE COLLISIONS!
		SetActorLocation(NewLocation, true);  // bSweep = true preserves collision detection
	}

	// Manual movement system - direct position change (disabled during knockback)
	if (!bIsKnockedBack)
	{
		ManualMovement(DeltaTime);
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
			// Triggered - called every frame while button held
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADoodleCharacter::Move);
			// Completed - called when button released - clear input
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ADoodleCharacter::ClearMovementInput);
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
		CurrentMovementInput = FVector2D::ZeroVector;
		return;
	}

	// Just store the input - actual movement happens in ManualMovement()
	CurrentMovementInput = Value.Get<FVector2D>();
}

void ADoodleCharacter::ClearMovementInput(const FInputActionValue& Value)
{
	// Called when button released - clear input for instant stop
	CurrentMovementInput = FVector2D::ZeroVector;
}

void ADoodleCharacter::ManualMovement(float DeltaTime)
{
	if (bIsFrozen)
	{
		return;
	}

	if (!Controller)
	{
		return;
	}

	// PURE DIRECT INPUT - NO VELOCITY TRACKING, NO INERTIA, NO PHYSICS
	// Movement is ONLY based on current button press - nothing else affects it

	if (CurrentMovementInput.IsNearlyZero())
	{
		// No input = no movement at all
		return;
	}

	// Calculate movement direction from input
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	FVector InputDirection = (ForwardDirection * CurrentMovementInput.Y) + (RightDirection * CurrentMovementInput.X);
	InputDirection.Normalize();

	// Calculate movement distance for this frame (distance = speed * time)
	FVector MovementDelta = InputDirection * MovementSpeed * DeltaTime;

	// Apply movement directly - no velocity, no acceleration, just pure position change
	FVector CurrentLocation = GetActorLocation();
	FVector NewLocation = CurrentLocation + MovementDelta;
	SetActorLocation(NewLocation, true);
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

	// CRITICAL: Clear movement input to prevent interference with knockback
	CurrentMovementInput = FVector2D::ZeroVector;

	// Set knockback state flag to prevent velocity clearing in Tick
	bIsKnockedBack = true;

	// Normalize direction to ensure consistent knockback force
	FVector KnockbackDirection = Direction.GetSafeNormal();

	// Apply knockback impulse using LaunchCharacter
	FVector KnockbackVelocity = KnockbackDirection * Force;
	LaunchCharacter(KnockbackVelocity, true, true);

	// End knockback after 0.5 seconds (allow physics to work)
	GetWorld()->GetTimerManager().SetTimer(KnockbackTimerHandle, this, &ADoodleCharacter::EndKnockback, 0.5f, false);

	UE_LOG(LogTemp, Warning, TEXT("Knockback applied! Direction: %s, Force: %.2f"), *KnockbackDirection.ToString(), Force);
	UE_LOG(LogTemp, Warning, TEXT("Movement input cleared, knockback state active"));
	UE_LOG(LogTemp, Warning, TEXT("Character velocity after knockback: %s"), *CharMovement->Velocity.ToString());
}

void ADoodleCharacter::EndKnockback()
{
	bIsKnockedBack = false;
	UE_LOG(LogTemp, Warning, TEXT("Knockback ended - player control restored"));
}

