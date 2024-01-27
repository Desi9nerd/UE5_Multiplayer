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
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);// ECC_SkeletalMesh는 Multiplayer.h에서 #define 정의하였음.
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
		);//Emitter를 붙일 Bone이 있으면 FName()에 적으면된다. 없으면 위처럼 빈칸으로 나두면된다.
	}
	
	if (HasAuthority()) // Server라면
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit); // Dynamic delegate 등록. UserObject는 발사체자신(this), 콜백함수는 &AProjectile::OnHit
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
	//	UGameplayStatics::SpawnEmitterAtLocation( // Blood 피 튀기기
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
	//	{	// 파티클 Spawn
	//		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	//	}
	//	if (IsValid(ImpactSound))
	//	{
	//		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	//	}
	//}

	Destroy(); // 충돌 후 충돌체 소멸
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
				DamageInnerRadius, // DamageInnerRadius
				DamageOuterRadius, // DamageOuterRadius
				1.0f, // DamageFalloff. InnerRadius->OuterRadius로 갈수록 데미지가 줄어드는 정도
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(), // IgnoreActors
				this, // DamageCauser. 여기서는 로켓(=ProjectileRocket)
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
	{	// 파티클 Spawn
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}

	if (IsValid(ImpactSound))
	{	
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}
