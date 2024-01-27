#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/** HitScan 방식의 무기
 *  ProjectileWeapon과 달리 총알이나 로켓이 날아가지 않는다.
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
	UParticleSystem* ImpactParticles; // 충돌 시 파티클

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound; // 피격 사운드

private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles; // Trail 궤적 파티클 이펙트

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash; // Muzzle 파티클 이펙트

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound; // 발사 사운드

};