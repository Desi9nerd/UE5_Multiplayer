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
	TWeakObjectPtr<APawn> FiringPawn = GetInstigator(); // GetInstigator()는 로켓을 쏘는 무기를 가지고 있는 Pawn을 리턴한다.
	if (FiringPawn.IsValid())
	{
		TWeakObjectPtr<AController> FiringController = FiringPawn->GetController();
		if (FiringController.IsValid())
		{
			//** 반경 데미지
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // World Context Object
				Damage, // BaseDamage
				10.0f, // MinimumDamage
				GetActorLocation(), // Origin. 로켓의 위치로 설정.
				200.0f, // DamageInnerRadius
				500.0f, // DamageOuterRadius
				1.0f, // DamageFalloff. InnerRadius->OuterRadius로 갈수록 데미지가 줄어드는 정도
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(), // IgnoreActors
				this, // DamageCauser. 여기서는 로켓(=ProjectileRocket)
				FiringController.Get() // InstigatorController
			);
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
