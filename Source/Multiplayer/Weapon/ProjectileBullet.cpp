#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);

	ProjectileMovementComponent->InitialSpeed = 1500.0f; // �Ѿ� �ӵ�. ���� ����.
	ProjectileMovementComponent->MaxSpeed = 1500.0f; // �Ѿ� �ִ� �ӵ�. ���� ����.
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f; // �Ѿ� �߷�. ���� ����.
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
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

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit); // Destroy() ȣ���� �θ� Projectile Ŭ�������� �Ͼ�� ������ �������� Super ȣ��
}
