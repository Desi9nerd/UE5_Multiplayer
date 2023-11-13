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

	//** Server-side Rewind�� ���Ǵ� ������
	bool bUseServerSideRewind = false;
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity; // �� �� ������ �� ����� ���� FVector_NetQuantize100 ���.
	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.0f; // ProjectileBullet(=�Ѿ�) �ӵ�
	UPROPERTY(EditAnywhere)
	float InitialRocketSpeed = 1500.0f; // ProjectileRocket(=����) �ӵ�

	// Only set this for Grenades and Rockets
	UPROPERTY(EditAnywhere)
	float Damage = 20.0f; // �߻�ü�� ���� ������
	// Doesn't matter for Grenades and Rockets
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.0f; // �߻�ü�� ���� ��弦 ������

protected:
	virtual void BeginPlay() override;
	void StartDestroyTimer();
	void DestroyTimerFinished();// TimeHandler�� �Բ� �� �ݹ� �Լ�
	void SpawnTrailSystem();	// �߻�ü Trail�� Spawn��Ű�� �Լ�
	void ExplodeDamage();

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem; // �߻�ü Trail ����Ʈ

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles; // �浹 �� ��ƼŬ

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;  // �浹 �� ����

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh; // �߻�ü �Ž� ex.�Ѿ�, ����

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.0f;
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.0f;

private:
	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer; // �߻�ü ������ ���̰� �� ��ƼŬ

	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;//Tracer�� Spawn��Ų �� ������ ����

	FTimerHandle DestroyTimer; // DestroyTimerFinshed() �Լ��� �Բ� ���� Ÿ�̸� ����

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0f; // �Ҹ꿡 �ɸ��� �ð�. �浹 �� 3�� �Ŀ� �Ҹ��Ѵ�.
	
};
