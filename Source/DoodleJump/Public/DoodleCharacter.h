#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DoodleCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class DOODLEJUMP_API ADoodleCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADoodleCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ActivateJumpBoost(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void FreezeCharacter(float Duration, AActor* AttachToActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplyKnockback(FVector Direction, float Force);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Settings")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Settings")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Settings")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float MovementSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float JumpForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float CustomGravityScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float CustomAirControl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float MouseSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float MinPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float MaxPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float SpringArmLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float RotationSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float JumpBoostMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Settings")
	float DefaultFreezeDuration;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void AutoJump();
	void AutoRotate(float DeltaTime);

	bool bIsFrozen;
	FTimerHandle FreezeTimerHandle;
	AActor* FreezeAttachmentActor;
	FVector FreezeRelativeOffset;  // Store offset from attachment actor
	void UnfreezeCharacter();
};
