#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	TWeakObjectPtr<APawn> FiringPawn = GetInstigator(); // GetInstigator()�� ������ ��� ���⸦ ������ �ִ� Pawn�� �����Ѵ�.
	if (FiringPawn.IsValid())
	{
		TWeakObjectPtr<AController> FiringController = FiringPawn->GetController();
		if (FiringController.IsValid())
		{
			//** �ݰ� ������
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // World Context Object
				Damage, // BaseDamage
				10.0f, // MinimumDamage
				GetActorLocation(), // Origin. ������ ��ġ�� ����.
				200.0f, // DamageInnerRadius
				500.0f, // DamageOuterRadius
				1.0f, // DamageFalloff. InnerRadius->OuterRadius�� ������ �������� �پ��� ����
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(), // IgnoreActors
				this, // DamageCauser. ���⼭�� ����(=ProjectileRocket)
				FiringController.Get() // InstigatorController
			);
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
