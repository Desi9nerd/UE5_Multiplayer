#pragma once
#include "CoreMinimal.h"
#include "Weapon.h"
#include "Flag.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYER_API AFlag : public AWeapon
{
	GENERATED_BODY()

public:
	FORCEINLINE FTransform GetInitialTransform() const { return InitialTransform; }

	AFlag();
	virtual void Dropped() override;
	void ResetFlag();

protected:
	virtual void OnEquipped() override;
	virtual void OnDropped() override;
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* FlagMesh;

	FTransform InitialTransform; // 깃발 최초 위치
};
