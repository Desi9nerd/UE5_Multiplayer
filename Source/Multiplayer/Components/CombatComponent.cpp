#include "CombatComponent.h"
#include "Multiplayer/Weapon/Weapon.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	//서버가 관리하는 모든 캐릭터에 모든 clinet가 Aiming 포즈를 볼 수 있도록 한다.
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)//RPC들은 _Implementation버젼 사용
{
	bAiming = bIsAiming;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);//replicated된 EquippedWeapon. EquippedWeapon이 변경되면 모든 client에 반영된다.
	DOREPLIFETIME(UCombatComponent, bAiming);//replicated 되도록 bAiming을 등록
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
	EquippedWeapon->SetOwner(Character); // 무기의 Owner을 Character로 설정
}
