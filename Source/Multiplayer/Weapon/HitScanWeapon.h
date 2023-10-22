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
	class UParticleSystem* ImpactParticles; // �浹 �� ��ƼŬ

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles; // Trail ���� ��ƼŬ ����Ʈ

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash; // Muzzle ��ƼŬ ����Ʈ

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound; // �߻� ����

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound; // �ǰ� ����
};
