#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

UCLASS()
class MULTIPLAYER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnPoint();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere) // BP에서 할당
	TArray<TSubclassOf<class APickup>> PickupClasses; // 다른 타입들의 Pickup들을 다 담는 배열

	UPROPERTY()
	APickup* SpawnedPickup;

	void SpawnPickup(); // Pickup 스폰
	void SpawnPickupTimerFinished(); // 지정시간이 지나고 불려져 Pickup 스폰을 시작

	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);

private:
	FTimerHandle SpawnPickupTimer;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;

};
