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
	SetWeaponState(EWeaponState::EWS_Dropped); // 무기상태 Dropped로 변경
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true); //소멸 시 World에 무기가 떨어지도록 DetachmentTransformRules 설정
	FlagMesh->DetachFromComponent(DetachRules); // FlagMesh를 캐릭터에서 떨어뜨린다.
	SetOwner(nullptr);  // 캐릭터가 무기를 떨어뜨린 후 무기의 Owner가 없도록 Owner를 nullptr로 설정
	BaseCharcterOwnerCharacter = nullptr; // 무기소유 캐릭터가 없도록 nullptr
	MainPlayerOwnerController = nullptr; // 무기소유 controller가 없도록 nullptr
}

void AFlag::OnEquipped()
{
	ShowPickupWidget(false); // PickupWidget 꺼줌
	GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // AreaSphere 충돌X

	FlagMesh->SetSimulatePhysics(false);
	FlagMesh->SetEnableGravity(false);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // FlagMesh 충돌X
	EnableCustomDepth(false); // 윤곽선X
}

void AFlag::OnDropped()
{
	if (HasAuthority()) // Server
	{
		GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	FlagMesh->SetSimulatePhysics(true);
	FlagMesh->SetEnableGravity(true);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // FlagMesh 충돌O
	FlagMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	FlagMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	FlagMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE); // 윤곽선 파란색
	FlagMesh->MarkRenderStateDirty();
	EnableCustomDepth(true); // 윤곽선O
}
