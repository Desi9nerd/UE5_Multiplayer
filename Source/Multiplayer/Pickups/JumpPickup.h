#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "JumpPickup.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYER_API AJumpPickup : public APickup
{
	GENERATED_BODY()

protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(EditAnywhere)
	float JumpZVelocityBuff = 4000.0f; // Jump Buff ���� �� JumpZVelocity ��

	UPROPERTY(EditAnywhere)
	float JumpBuffTime = 30.0f; // Jump Buff�� ���ӵǴ� �ð�
};
