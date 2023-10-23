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
	void StartDestroyTimer();
	void DestroyTimerFinished();// TimeHandler와 함께 쓸 콜백 함수
	void SpawnTrailSystem();	// 발사체 Trail를 Spawn시키는 함수
	void ExplodeDamage();

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere) // 하위 클래스에서 접근할 수 있도록 protected
	float Damage = 20.0f; // 발사체가 입힐 데미지

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem; // 발사체 Trail 이펙트

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles; // 충돌 후 파티클

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;  // 충돌 후 사운드

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh; // 발사체 매쉬 ex.총알, 로켓

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.0f;
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.0f;

private:
	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer; // 발사체 궤적을 보이게 할 파티클

	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;//Tracer를 Spawn시킨 후 저장할 변수

	FTimerHandle DestroyTimer; // DestroyTimerFinshed() 함수와 함께 쓰일 타이머 변수

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0f; // 소멸에 걸리는 시간. 충돌 후 3초 후에 소멸한다.
	
};
