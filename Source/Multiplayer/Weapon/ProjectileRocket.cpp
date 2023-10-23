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
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	SpawnTrailSystem();

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

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner()) return; // 로켓에 자기 자신이 맞는 경우 예외 처리

	ExplodeDamage();

	StartDestroyTimer();

	if (IsValid(ImpactParticles))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform()); // 로켓 충돌 파티클 스폰
	}
	if (IsValid(ImpactSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation()); // 로켓 충돌 사운드 재생
	}
	if (IsValid(ProjectileMesh))
	{
		ProjectileMesh->SetVisibility(false); // 발사체(로켓) 메쉬 꺼줌
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
