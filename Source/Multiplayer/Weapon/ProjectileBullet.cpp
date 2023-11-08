#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);

	ProjectileMovementComponent->InitialSpeed = InitialSpeed; // 총알 속도
	ProjectileMovementComponent->MaxSpeed = InitialSpeed; // 총알 최대 속도
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

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	FPredictProjectilePathParams PathParams; // Projectile Path 계산에 필요한 값들을 담는 변수
	PathParams.bTraceWithChannel = true; // 특정 ECollisionChannel을 사용할 수 있도록 true 설정
	PathParams.bTraceWithCollision = true; // 쏜 Trace의 Hit Event가 발생하도록 true 설정
	PathParams.DrawDebugTime = 5.0f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration; // DrawDebugTime 동안만 보여주도록 설정
	PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed; // 발사속도 = 전방벡터 * 총알속도
	PathParams.MaxSimTime = 4.0f; // 발사체(=ProjectileBullet)가 공중에 머물 수 있는 최대시간
	PathParams.ProjectileRadius = 5.0f; // SphereTrace의 반지름 값
	PathParams.SimFrequency = 30.0f; // 빈도수. 높을수록 SphereTrace가 많아진다.
	PathParams.StartLocation = GetActorLocation(); // 시작위치
	PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	PathParams.ActorsToIgnore.Add(this); // Projectile Path Trace 계산에 무시되는 Actor들

	FPredictProjectilePathResult PathResult; // Projectile Path 결과값

	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult); // PathParams 값으로 ProjectilePath을 계산하여 PathResult로 결과값을 업데이트한다.
}
