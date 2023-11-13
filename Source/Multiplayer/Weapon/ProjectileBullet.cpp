#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Multiplayer/Components/LagCompensationComponent.h"
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

#if WITH_EDITOR // Editor에서만 override되도록 #if~#endif 사용. 컴파일 타임 수행. 런타임 실행X
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	// Property가 변경되었을때 변경된 것들 중 InitialSpeed란 이름이 있다면 
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (IsValid(ProjectileMovementComponent))
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	TWeakObjectPtr<ABaseCharacter> OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
	if (OwnerCharacter.IsValid()) // 총알을 맞춘 대상이 캐릭터(=적 플레이어)라면
	{
		TWeakObjectPtr<AMainPlayerController> OwnerController = Cast<AMainPlayerController>(OwnerCharacter->Controller);
		if (OwnerController.IsValid())
		{
			if (OwnerCharacter->HasAuthority() && bUseServerSideRewind == false) // Server && SSR X
			{
				const float DamageToCause = Hit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage; // 피격된 본이 "head"라면 HeadShotDamage, 아니면 Damage

				UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerController.Get(), this, UDamageType::StaticClass());

				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}
			TWeakObjectPtr<ABaseCharacter> HitCharacter = Cast<ABaseCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled() && HitCharacter.IsValid()) // Client && SSR O
			{
				OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
					HitCharacter.Get(),
					TraceStart,
					InitialVelocity,
					OwnerController->GetServerTime() - OwnerController->SingleTripTime
				);
			}
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit); // Destroy() 호출이 부모 Projectile 클래스에서 일어나기 때문에 마지막에 Super 호출
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	/*
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
	*/
}
