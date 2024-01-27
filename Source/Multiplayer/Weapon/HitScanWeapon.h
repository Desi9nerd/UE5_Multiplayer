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
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactBlood;
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles; // �浹 �� ��ƼŬ

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound; // �ǰ� ����

private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles; // Trail ���� ��ƼŬ ����Ʈ

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash; // Muzzle ��ƼŬ ����Ʈ

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound; // �߻� ����

};