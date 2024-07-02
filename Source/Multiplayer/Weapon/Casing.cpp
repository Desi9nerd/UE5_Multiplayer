#include "Casing.h"
#include "Kismet/GameplayStatics.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);//ī�޶�� �浹�� �Ͼ�� �ʵ��� ����
	CasingMesh->SetSimulatePhysics(true);// ���� ����
	CasingMesh->SetEnableGravity(true);  // �߷� ����
	CasingMesh->SetNotifyRigidBodyCollision(true); // �Ѿ�ź�ǰ� �ε�ĥ �� �ֵ��� ����. BP������ "Simulation Generates Hit Events"��� �Ҹ���.
	ShellEjectionImpulse = 10.0f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);//�Ѿ�ź�ǰ� �������´�
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (IsValid(ShellSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	Destroy(); // �Ҹ� �����ش�.
}
