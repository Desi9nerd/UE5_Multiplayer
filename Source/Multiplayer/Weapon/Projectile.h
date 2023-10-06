#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class MULTIPLAYER_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere) // 하위 클래스에서 접근할 수 있도록 protected
	float Damage = 20.0f; // 발사체가 입힐 데미지

private:
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer; // 발사체 궤적을 보이게 할 파티클

	class UParticleSystemComponent* TracerComponent;//Tracer를 Spawn시킨 후 저장할 변수

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles; // 충돌 후 파티클

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;  // 충돌 후 사운드

public:

};
