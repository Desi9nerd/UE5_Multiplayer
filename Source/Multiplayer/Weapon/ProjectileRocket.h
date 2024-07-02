#pragma once
#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

class URocketMovementComponent;

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
	TObjectPtr<USoundCue> ProjectileLoop; // ������ ���ư��� ����

	UPROPERTY()
	TObjectPtr<UAudioComponent> ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundAttenuation> LoopingSoundAttenuation; // ������ ���ư��鼭 ���� �־���

	UPROPERTY(VisibleAnywhere)// ������ ProjectileMovementComponent ��� �̰��� ���.
	TObjectPtr<URocketMovementComponent> RocketMovementComponent;
	
};
