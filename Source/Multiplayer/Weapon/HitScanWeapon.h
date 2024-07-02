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
	TObjectPtr<UParticleSystem> ImpactBlood;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactParticles; // �浹 �� ��ƼŬ

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> HitSound; // �ǰ� ����

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> BeamParticles; // Trail ���� ��ƼŬ ����Ʈ

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> MuzzleFlash; // Muzzle ��ƼŬ ����Ʈ

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> FireSound; // �߻� ����
};