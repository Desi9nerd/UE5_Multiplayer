#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/Multiplayer.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ProfilingDebugging/CookStats.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);// ECC_SkeletalMesh�� Multiplayer.h���� #define �����Ͽ���.
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(Tracer))
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);//Emitter�� ���� Bone�� ������ FName()�� ������ȴ�. ������ ��ó�� ��ĭ���� ���θ�ȴ�.
	}
	
	if (HasAuthority()) // Server���
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit); // Dynamic delegate ���. UserObject�� �߻�ü�ڽ�(this), �ݹ��Լ��� &AProjectile::OnHit
		CollisionBox->IgnoreActorWhenMoving(Owner, true); 
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	TWeakObjectPtr<ABaseCharacter> HitCharacter = Cast<ABaseCharacter>(Hit.GetActor());
	if (HitCharacter.IsValid())
	{
		bBloodParticle = true;
	}

	//if(HitCharacter.IsValid())
	//{
	//	UGameplayStatics::SpawnEmitterAtLocation( // Blood �� Ƣ���
	//		Hit.GetActor(),
	//		ImpactBlood,
	//		Hit.ImpactPoint,
	//		UKismetMathLibrary::MakeRotFromXY(Hit.Normal.XAxisVector, Hit.Normal.YAxisVector),
	//		UKismetMathLibrary::MakeRotationFromAxes(Hit.Normal.XAxisVector, Hit.Normal.YAxisVector, Hit.Normal.ZAxisVector),
	//		FVector(0.4f, 0.4f, 0.4f),
	//		true
	//	);
	//}
	//else
	//{
	//	if (IsValid(ImpactParticles))
	//	{	// ��ƼŬ Spawn
	//		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	//	}
	//	if (IsValid(ImpactSound))
	//	{
	//		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	//	}
	//}

	Destroy(); // �浹 �� �浹ü �Ҹ�
}

void AProjectile::SpawnTrailSystem()
{
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
}

void AProjectile::ExplodeDamage()
{
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
				DamageInnerRadius, // DamageInnerRadius
				DamageOuterRadius, // DamageOuterRadius
				1.0f, // DamageFalloff. InnerRadius->OuterRadius�� ������ �������� �پ��� ����
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(), // IgnoreActors
				this, // DamageCauser. ���⼭�� ����(=ProjectileRocket)
				FiringController.Get() // InstigatorController
			);
		}
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimerFinished,
		DestroyTime
	);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (bBloodParticle && IsValid(ImpactBlood))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactBlood, GetActorTransform());
		bBloodParticle = false;
	}
	else if (IsValid(ImpactParticles))
	{	// ��ƼŬ Spawn
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}

	if (IsValid(ImpactSound))
	{	
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}
