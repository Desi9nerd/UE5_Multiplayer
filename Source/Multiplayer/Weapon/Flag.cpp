#include "Flag.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

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

void AFlag::OnEquipped()
{
	ShowPickupWidget(false); // PickupWidget ����
	GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // AreaSphere �浹X

	FlagMesh->SetSimulatePhysics(false);
	FlagMesh->SetEnableGravity(false);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // FlagMesh �浹X
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
