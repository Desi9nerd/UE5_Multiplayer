#pragma once
#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

class AProjectile;

UCLASS()
class MULTIPLAYER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

private:
	UPROPERTY(EditAnywhere) // Replicated O
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere) // Replicated X. Local������ Spawn�ȴ�
	TSubclassOf<AProjectile> ServerSideRewindProjectileClass;
};
