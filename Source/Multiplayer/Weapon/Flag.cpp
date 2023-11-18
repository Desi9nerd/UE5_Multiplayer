#include "Flag.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Multiplayer/Character/BaseCharacter.h"

AFlag::AFlag()
{
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	SetRootComponent(FlagMesh);
	GetAreaSphere()->SetupAttachment(FlagMesh);
	GetPickupWidget()->SetupAttachment(FlagMesh);
	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFlag::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped); // ������� Dropped�� ����
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true); //�Ҹ� �� World�� ���Ⱑ ���������� DetachmentTransformRules ����
	FlagMesh->DetachFromComponent(DetachRules); // FlagMesh�� ĳ���Ϳ��� ����߸���.
	SetOwner(nullptr);  // ĳ���Ͱ� ���⸦ ����߸� �� ������ Owner�� ������ Owner�� nullptr�� ����
	BaseCharcterOwnerCharacter = nullptr; // ������� ĳ���Ͱ� ������ nullptr
	MainPlayerOwnerController = nullptr; // ������� controller�� ������ nullptr
}

void AFlag::ResetFlag() // ��� �ʱ�ȭ
{
	TWeakObjectPtr<ABaseCharacter> FlagBearer = Cast<ABaseCharacter>(GetOwner());
	if (FlagBearer.IsValid())
	{
		FlagBearer->SetHoldingTheFlag(false);
		FlagBearer->SetOverlappingWeapon(nullptr);
		FlagBearer->UnCrouch(); // ĳ���� �� �ִ� ���·� ����
	}

	if (false == HasAuthority()) return; // Client�̸� ����

	//** Server���� ����
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	FlagMesh->DetachFromComponent(DetachRules);
	SetWeaponState(EWeaponState::EWS_Initial); // ���(=����)���¸� ó�� ���·� �ʱ�ȭ
	GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetAreaSphere()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	SetOwner(nullptr); 
	BaseCharcterOwnerCharacter = nullptr;
	MainPlayerOwnerController = nullptr;

	SetActorTransform(InitialTransform); // ��� ���� ��ġ�� �ǵ�����
}

void AFlag::OnEquipped()
{
	ShowPickupWidget(false); // PickupWidget ����
	GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // AreaSphere �浹X

	FlagMesh->SetSimulatePhysics(false);
	FlagMesh->SetEnableGravity(false);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	EnableCustomDepth(false); // ������X
}

void AFlag::OnDropped()
{
	if (HasAuthority()) // Server
	{
		GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	FlagMesh->SetSimulatePhysics(true);
	FlagMesh->SetEnableGravity(true);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // FlagMesh �浹O
	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	FlagMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE); // ������ �Ķ���
	FlagMesh->MarkRenderStateDirty();
	EnableCustomDepth(true); // ������O
}

void AFlag::BeginPlay()
{
	Super::BeginPlay();

	InitialTransform = GetActorTransform();
}
