#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);

	ProjectileMovementComponent->InitialSpeed = 1500.0f; // 총알 속도. 변경 가능.
	ProjectileMovementComponent->MaxSpeed = 1500.0f; // 총알 최대 속도. 변경 가능.
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f; // 총알 중력. 변경 가능.
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	TWeakObjectPtr<ACharacter> OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter.IsValid()) // 총알을 맞춘 대상이 캐릭터(=적 플레이어)라면
	{
		TWeakObjectPtr<AController> OwnerController = OwnerCharacter->Controller;
		if (OwnerController.IsValid())
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController.Get(), this, UDamageType::StaticClass());
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit); // Destroy() 호출이 부모 Projectile 클래스에서 일어나기 때문에 마지막에 Super 호출
}
