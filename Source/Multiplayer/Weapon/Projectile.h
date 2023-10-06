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

	UPROPERTY(EditAnywhere) // ���� Ŭ�������� ������ �� �ֵ��� protected
	float Damage = 20.0f; // �߻�ü�� ���� ������

private:
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer; // �߻�ü ������ ���̰� �� ��ƼŬ

	class UParticleSystemComponent* TracerComponent;//Tracer�� Spawn��Ų �� ������ ����

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles; // �浹 �� ��ƼŬ

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;  // �浹 �� ����

public:

};
