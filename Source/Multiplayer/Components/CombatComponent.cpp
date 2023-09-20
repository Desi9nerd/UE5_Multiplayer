#include "CombatComponent.h"
#include "Multiplayer/Weapon/Weapon.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Engine/SkeletalMeshSocket.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);//무기 상태를 Equipped(=장착)으로 변경
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));//소켓을 변수로 저장
	if (IsValid(HandSocket)) // 해당 소켓이 존재하면
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh()); //무기를 해당 소켓에 붙여준다.
	}
	EquippedWeapon->SetOwner(Character);	// 무기의 Owner을 Character로 설정
	EquippedWeapon->ShowPickupWidget(false);// 무기 장착 시 PickupWidget을 꺼준다
}