#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UBoxComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UProjectileMovementComponent;
class USoundCue;

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

	// ����ź & ���� ��ó
	UPROPERTY(EditAnywhere)
	float Damage = 20.0f; // �߻�ü�� ���� ������
	
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
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> TrailSystem; // �߻�ü Trail ����Ʈ

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TrailSystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactBlood; // �浹 �� �� Ƣ��

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactParticles; // �浹 �� ��ƼŬ

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ImpactSound;  // �浹 �� ����

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ProjectileMesh; // �߻�ü �Ž� ex.�Ѿ�, ����

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.0f;
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.0f;

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> Tracer; // �߻�ü ������ ���̰� �� ��ƼŬ

	UPROPERTY()
	TObjectPtr<UParticleSystemComponent> TracerComponent;//Tracer�� Spawn��Ų �� ������ ����

	FTimerHandle DestroyTimer; // DestroyTimerFinshed() �Լ��� �Բ� ���� Ÿ�̸� ����

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0f; // �Ҹ꿡 �ɸ��� �ð�. �浹 �� 3�� �Ŀ� �Ҹ��Ѵ�.

	bool bBloodParticle = false;
};
