#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
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
