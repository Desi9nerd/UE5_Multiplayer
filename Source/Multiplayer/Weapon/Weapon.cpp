#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Multiplayer/Character/BaseCharacter.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh.Get());

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);//WeaponMesh�� channel �浹�� block ó��
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);//Pawn�� ���, �浹 ����, ���Ⱑ Pawn(ex.�� ĳ����)�� �ε������� ����� �� �ֵ��� ����.
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);//ó�� ���Ⱑ �������� No Collision

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);//channel �浹x, AreaSphere�� �浹x
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	//������ Weapon ��ü�� ������ �� �ֵ��� �Ѵ�
	if (HasAuthority()) //HasAuthority()�� GetLocalRole() == ENetRole::ROLE_Authority�� ����.
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap); //���� AreaSphere�� ��ġ�� OnSphereOverlap()�Լ� Delegateȣ��
	}
	if (PickupWidget.IsValid())
	{
		PickupWidget->SetVisibility(false); //PickupWidget�� ���ְ� �����Ѵ�.
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter.IsValid() && PickupWidget.IsValid())
	{
		PickupWidget->SetVisibility(true); //�����ϸ� PickupWidget�� ���ش�.
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
