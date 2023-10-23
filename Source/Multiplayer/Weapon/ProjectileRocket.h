#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

/** 로켓 발사체
 * 
 */
UCLASS()
class MULTIPLAYER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();
	virtual void Destroyed() override;

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop; // 로켓이 날아가는 사운드

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation; // 로켓이 날아가면서 사운드 멀어짐

	UPROPERTY(VisibleAnywhere)// 로켓은 ProjectileMovementComponent 대신 이것을 사용.
	class URocketMovementComponent* RocketMovementComponent;

private:
};
