#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

/** ���� �߻�ü
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
	void DestroyTimerFinished(); // TimeHandler�� �Բ� �� �ݹ� �Լ�

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem; // ���� Trail ����Ʈ

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop; // ������ ���ư��� ����

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation; // ������ ���ư��鼭 ���� �־���

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	FTimerHandle DestroyTimer; // DestroyTimerFinshed() �Լ��� �Բ� ���� Ÿ�̸� ����

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0f; // �Ҹ꿡 �ɸ��� �ð�. �浹 �� 3�� �Ŀ� �Ҹ��Ѵ�.
};
