#include "Casing.h"
#include "Kismet/GameplayStatics.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);//카메라와 충돌이 일어나지 않도록 설정
	CasingMesh->SetSimulatePhysics(true);// 물리 적용
	CasingMesh->SetEnableGravity(true);  // 중력 적용
	CasingMesh->SetNotifyRigidBodyCollision(true); // 총알탄피가 부딪칠 수 있도록 설정. BP에서는 "Simulation Generates Hit Events"라고 불린다.
	ShellEjectionImpulse = 10.0f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);//총알탄피가 빠져나온다
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (IsValid(ShellSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	Destroy(); // 소멸 시켜준다.
}
