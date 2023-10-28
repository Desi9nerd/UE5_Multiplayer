#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "SpeedPickup.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYER_API ASpeedPickup : public APickup
{
	GENERATED_BODY()

protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(EditAnywhere)
	float BaseSpeedBuff = 1600.0f;

	UPROPERTY(EditAnywhere)
	float CrouchSpeedBuff = 850.0f;

	UPROPERTY(EditAnywhere)
	float SpeedBuffTime = 30.0f;
};
