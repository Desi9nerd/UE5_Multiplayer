#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

UCLASS()
class MULTIPLAYER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

private:
	UPROPERTY(EditAnywhere) // Replicated O
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere) // Replicated X. Local에서만 Spawn된다
	TSubclassOf<AProjectile> ServerSideRewindProjectileClass;
};
