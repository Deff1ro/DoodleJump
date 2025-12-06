#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BreakablePlatform.generated.h"

class USkeletalMeshComponent;
class UBoxComponent;
class UAnimSequenceBase;

UCLASS()
class DOODLEJUMP_API ABreakablePlatform : public AActor
{
	GENERATED_BODY()

public:
	ABreakablePlatform();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* PlatformMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	float BreakDelay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	bool bIsBroken;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	UAnimSequenceBase* BreakAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	bool bUsePhysics;

private:
	FTimerHandle BreakTimerHandle;
	FTimerHandle PhysicsTimerHandle;

	UFUNCTION()
	void OnPlatformHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void BreakPlatform();
};
