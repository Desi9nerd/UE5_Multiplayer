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

#if WITH_EDITOR // Editor������ override�ǵ��� #if~#endif ���. ������ Ÿ�� ����
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& Event) override;
#endif

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop; // ������ ���ư��� ����

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation; // ������ ���ư��鼭 ���� �־���

	UPROPERTY(VisibleAnywhere)// ������ ProjectileMovementComponent ��� �̰��� ���.
	class URocketMovementComponent* RocketMovementComponent;

private:
};
