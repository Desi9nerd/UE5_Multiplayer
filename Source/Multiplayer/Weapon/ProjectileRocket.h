#pragma once
#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

class URocketMovementComponent;

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

#if WITH_EDITOR // Editor에서만 override되도록 #if~#endif 사용. 컴파일 타임 수행
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& Event) override;
#endif

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ProjectileLoop; // 로켓이 날아가는 사운드

	UPROPERTY()
	TObjectPtr<UAudioComponent> ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundAttenuation> LoopingSoundAttenuation; // 로켓이 날아가면서 사운드 멀어짐

	UPROPERTY(VisibleAnywhere)// 로켓은 ProjectileMovementComponent 대신 이것을 사용.
	TObjectPtr<URocketMovementComponent> RocketMovementComponent;
	
};
