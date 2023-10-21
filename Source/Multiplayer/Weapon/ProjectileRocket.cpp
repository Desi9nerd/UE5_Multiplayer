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

	RocketMovementComponent->InitialSpeed = 1500.0f; // ���� �ӵ�. ���� ����.
	RocketMovementComponent->MaxSpeed = 1500.0f; // ���� �ִ� �ӵ�. ���� ����.
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
	if (OtherActor == GetOwner()) return; // ���Ͽ� �ڱ� �ڽ��� �´� ��� ���� ó��

	TWeakObjectPtr<APawn> FiringPawn = GetInstigator(); // GetInstigator()�� ������ ��� ���⸦ ������ �ִ� Pawn�� �����Ѵ�.
	if (FiringPawn.IsValid())
	{
		TWeakObjectPtr<AController> FiringController = FiringPawn->GetController();
		if (FiringController.IsValid() && HasAuthority())
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

	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectileRocket::DestroyTimerFinished,
		DestroyTime
	);

	if (IsValid(ImpactParticles))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform()); // ���� �浹 ��ƼŬ ����
	}
	if (IsValid(ImpactSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation()); // ���� �浹 ���� ���
	}
	if (IsValid(RocketMesh))
	{
		RocketMesh->SetVisibility(false); // ���� �޽� ����
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
