#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	TWeakObjectPtr<ACharacter> OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter.IsValid()) // �Ѿ��� ���� ����� ĳ����(=�� �÷��̾�)���
	{
		TWeakObjectPtr<AController> OwnerController = OwnerCharacter->Controller;
		if (OwnerController.IsValid())
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController.Get(), this, UDamageType::StaticClass());
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
