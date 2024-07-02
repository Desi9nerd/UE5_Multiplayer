#pragma once
#include "CoreMinimal.h"
#include "Pickup.h"
#include "ShieldPickup.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYER_API AShieldPickup : public APickup
{
	GENERATED_BODY()

protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:

	UPROPERTY(EditAnywhere)
	float ShieldReplenishAmount = 100.0f; // Shild ��ġ�� ä��� ����

	UPROPERTY(EditAnywhere)
	float ShieldReplenishTime = 5.0f; // Shield�� ���� �ð�
};
