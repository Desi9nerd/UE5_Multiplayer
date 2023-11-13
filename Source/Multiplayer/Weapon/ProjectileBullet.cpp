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

	ProjectileMovementComponent->InitialSpeed = InitialSpeed; // �Ѿ� �ӵ�
	ProjectileMovementComponent->MaxSpeed = InitialSpeed; // �Ѿ� �ִ� �ӵ�
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f; // �Ѿ� �߷�. ���� ����.
}

#if WITH_EDITOR // Editor������ override�ǵ��� #if~#endif ���. ������ Ÿ�� ����. ��Ÿ�� ����X
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	// Property�� ����Ǿ����� ����� �͵� �� InitialSpeed�� �̸��� �ִٸ� 
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
	if (OwnerCharacter.IsValid()) // �Ѿ��� ���� ����� ĳ����(=�� �÷��̾�)���
	{
		TWeakObjectPtr<AMainPlayerController> OwnerController = Cast<AMainPlayerController>(OwnerCharacter->Controller);
		if (OwnerController.IsValid())
		{
			if (OwnerCharacter->HasAuthority() && bUseServerSideRewind == false) // Server && SSR X
			{
				const float DamageToCause = Hit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage; // �ǰݵ� ���� "head"��� HeadShotDamage, �ƴϸ� Damage

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

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit); // Destroy() ȣ���� �θ� Projectile Ŭ�������� �Ͼ�� ������ �������� Super ȣ��
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	/*
	FPredictProjectilePathParams PathParams; // Projectile Path ��꿡 �ʿ��� ������ ��� ����
	PathParams.bTraceWithChannel = true; // Ư�� ECollisionChannel�� ����� �� �ֵ��� true ����
	PathParams.bTraceWithCollision = true; // �� Trace�� Hit Event�� �߻��ϵ��� true ����
	PathParams.DrawDebugTime = 5.0f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration; // DrawDebugTime ���ȸ� �����ֵ��� ����
	PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed; // �߻�ӵ� = ���溤�� * �Ѿ˼ӵ�
	PathParams.MaxSimTime = 4.0f; // �߻�ü(=ProjectileBullet)�� ���߿� �ӹ� �� �ִ� �ִ�ð�
	PathParams.ProjectileRadius = 5.0f; // SphereTrace�� ������ ��
	PathParams.SimFrequency = 30.0f; // �󵵼�. �������� SphereTrace�� ��������.
	PathParams.StartLocation = GetActorLocation(); // ������ġ
	PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	PathParams.ActorsToIgnore.Add(this); // Projectile Path Trace ��꿡 ���õǴ� Actor��

	FPredictProjectilePathResult PathResult; // Projectile Path �����

	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult); // PathParams ������ ProjectilePath�� ����Ͽ� PathResult�� ������� ������Ʈ�Ѵ�.
	*/
}
