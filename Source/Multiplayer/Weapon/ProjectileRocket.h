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
	void DestroyTimerFinished(); // TimeHandler와 함께 쓸 콜백 함수

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem; // 로켓 Trail 이펙트

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop; // 로켓이 날아가는 사운드

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation; // 로켓이 날아가면서 사운드 멀어짐

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	FTimerHandle DestroyTimer; // DestroyTimerFinshed() 함수와 함께 쓰일 타이머 변수

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0f; // 소멸에 걸리는 시간. 충돌 후 3초 후에 소멸한다.
};
