#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
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
