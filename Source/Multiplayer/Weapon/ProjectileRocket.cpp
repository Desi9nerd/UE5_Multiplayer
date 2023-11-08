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

	RocketMovementComponent->InitialSpeed = InitialRocketSpeed; // ���� �ӵ�.
	RocketMovementComponent->MaxSpeed = InitialRocketSpeed; // ���� �ִ� �ӵ�
	RocketMovementComponent->ProjectileGravityScale = 0.2f; // ���� �߷�. ���� ����.
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay(); 

	// �θ��� Projectile.h.cpp���� Server�� OnHit�� ó���Ǳ� ������ ���⼭ Client�� OnHit�� ó���Ѵ�.
	if (HasAuthority() == false) // Client
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit); // CollisionBox�� projectile.h�� ����� ����. Dynamic delegate ���. UserObject�� �߻�ü�ڽ�(this), �ݹ��Լ��� &AProjectile::OnHit
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

#if WITH_EDITOR // Editor������ override�ǵ��� #if~#endif ���. ������ Ÿ�� ����. ��Ÿ�� ����X
void AProjectileRocket::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	// Property�� ����Ǿ����� ����� �͵� �� InitialRocketSpeed�� �̸��� �ִٸ� 
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileRocket, InitialRocketSpeed))
	{
		if (IsValid(RocketMovementComponent))
		{
			RocketMovementComponent->InitialSpeed = InitialRocketSpeed;
			RocketMovementComponent->MaxSpeed = InitialRocketSpeed;
		}
	}
}
#endif

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner()) return; // ���Ͽ� �ڱ� �ڽ��� �´� ��� ���� ó��

	ExplodeDamage();

	StartDestroyTimer();

	if (IsValid(ImpactParticles))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform()); // ���� �浹 ��ƼŬ ����
	}
	if (IsValid(ImpactSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation()); // ���� �浹 ���� ���
	}
	if (IsValid(ProjectileMesh))
	{
		ProjectileMesh->SetVisibility(false); // �߻�ü(����) �޽� ����
	}
	if (IsValid(CollisionBox))
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // �浹x
	}
	if (IsValid(TrailSystemComponent) && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate(); // ��ƼŬ ����
	}
	if (IsValid(ProjectileLoopComponent) && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop(); // ���� ������
	}
	
}
