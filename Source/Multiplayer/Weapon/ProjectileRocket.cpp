#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);

	RocketMovementComponent->InitialSpeed = 1500.0f; // 로켓 속도. 변경 가능.
	RocketMovementComponent->MaxSpeed = 1500.0f; // 로켓 최대 속도. 변경 가능.
	RocketMovementComponent->ProjectileGravityScale = 0.2f; // 로켓 중력. 변경 가능.
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay(); 

	// 부모인 Projectile.h.cpp에서 Server의 OnHit는 처리되기 때문에 여기서 Client쪽 OnHit을 처리한다.
	if (HasAuthority() == false) // Client
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit); // CollisionBox는 projectile.h에 선언된 변수. Dynamic delegate 등록. UserObject는 발사체자신(this), 콜백함수는 &AProjectile::OnHit
	}

	if (IsValid(TrailSystem))
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
	if (IsValid(ProjectileLoop) && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.0f,
			1.0f,
			0.0f,
			LoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
}

void AProjectileRocket::Destroyed()
{
	Super::Destroyed();
}

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner()) return; // 로켓에 자기 자신이 맞는 경우 예외 처리

	TWeakObjectPtr<APawn> FiringPawn = GetInstigator(); // GetInstigator()는 로켓을 쏘는 무기를 가지고 있는 Pawn을 리턴한다.
	if (FiringPawn.IsValid())
	{
		TWeakObjectPtr<AController> FiringController = FiringPawn->GetController();
		if (FiringController.IsValid() && HasAuthority())
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

	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectileRocket::DestroyTimerFinished,
		DestroyTime
	);

	if (IsValid(ImpactParticles))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform()); // 로켓 충돌 파티클 스폰
	}
	if (IsValid(ImpactSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation()); // 로켓 충돌 사운드 재생
	}
	if (IsValid(RocketMesh))
	{
		RocketMesh->SetVisibility(false); // 로켓 메쉬 꺼줌
	}
	if (IsValid(CollisionBox))
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 충돌x
	}
	if (IsValid(TrailSystemComponent) && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate(); // 파티클 꺼줌
	}
	if (IsValid(ProjectileLoopComponent) && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop(); // 로켓 멈춰줌
	}
	
}
