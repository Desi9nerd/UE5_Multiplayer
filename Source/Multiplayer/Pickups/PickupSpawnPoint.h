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

	UPROPERTY(EditAnywhere) // BP���� �Ҵ�
	TArray<TSubclassOf<class APickup>> PickupClasses; // �ٸ� Ÿ�Ե��� Pickup���� �� ��� �迭

	UPROPERTY()
	APickup* SpawnedPickup;

	void SpawnPickup(); // Pickup ����
	void SpawnPickupTimerFinished(); // �����ð��� ������ �ҷ��� Pickup ������ ����

	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);

private:
	FTimerHandle SpawnPickupTimer;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;

};
