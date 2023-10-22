#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/** HitScan ����� ����
 *  ProjectileWeapon�� �޸� �Ѿ��̳� ������ ���ư��� �ʴ´�.
 */
UCLASS()
class MULTIPLAYER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

protected:
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);

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

	//** Trace end with scatter
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.0f; // ������ SphereRadius������ �Ÿ�

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.0f; // ������ ��ź������ �̿�� SphereRadius

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false; // ������ ��ź���� true/false
	//**

};
