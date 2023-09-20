#include "Weapon.h"
#include "Components/SphereComponent.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);//WeaponMesh�� channel �浹�� block ó��
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);//Pawn�� ���, �浹 ����, ���Ⱑ Pawn(ex.�� ĳ����)�� �ε������� ����� �� �ֵ��� ����.
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);//ó�� ���Ⱑ �������� No Collision

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);//channel �浹x, AreaSphere�� �浹x
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//������ Weapon ��ü�� ������ �� �ֵ��� �Ѵ�
	if (HasAuthority()) //HasAuthority()�� GetLocalRole() == ENetRole::ROLE_Authority�� ����.
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}