#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

private:
	UPROPERTY(EditAnywhere)
	float Damage = 20.0f; 

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles; // 충돌 시 파티클

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles; // Trail 궤적 파티클 이펙트

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash; // Muzzle 파티클 이펙트

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound; // 발사 사운드

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound; // 피격 사운드
};
