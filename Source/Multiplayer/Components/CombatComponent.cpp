#include "CombatComponent.h"
#include "Multiplayer/Weapon/Weapon.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BaseWalkSpeed = 600.0f;
	AimWalkSpeed = 450.0f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{	// ĳ������ Walk�ִ�ӵ��� BaseWalkSpeed�� ����.
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	//������ �����ϴ� ��� ĳ���Ϳ� ��� clinet�� Aiming ��� �� �� �ֵ��� �Ѵ�.
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		//ĳ���Ͱ� ����(=Aiming)���̶�� MaxWalkSpeed�� AimWalkSpeed�� �ƴϸ� BaseWalkSpeed�� ����.
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)//RPC���� _Implementation���� ���
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character) //�������� ��
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;//�������� �� bOrientRotationMovement ���ش�.
		Character->bUseControllerRotationYaw = true;//���콺 �¿�ȸ�� �� ĳ���Ͱ� ȸ���ϸ� ����ؼ� ������ �ٶ󺸵��� true ����.
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (EquippedWeapon == nullptr) return; //���� ���Ⱑ ���ٸ� return

	if (Character && bFireButtonPressed)
	{
		Character->PlayFireMontage(bAiming); // �߻� ��Ÿ�� Play
		EquippedWeapon->Fire(); // ���� ���� �߻�
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);//replicated�� EquippedWeapon. EquippedWeapon�� ����Ǹ� ��� client�� �ݿ��ȴ�.
	DOREPLIFETIME(UCombatComponent, bAiming);//replicated �ǵ��� bAiming�� ���
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);//���� ���¸� Equipped(=����)���� ����
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));//������ ������ ����
	if (IsValid(HandSocket)) // �ش� ������ �����ϸ�
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh()); //���⸦ �ش� ���Ͽ� �ٿ��ش�.
	}
	EquippedWeapon->SetOwner(Character); // ������ Owner�� Character�� ����
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;//�������� �� bOrientRotationMovement ���ش�.
	Character->bUseControllerRotationYaw = true;//���콺 �¿�ȸ�� �� ĳ���Ͱ� ȸ���ϸ� ����ؼ� ������ �ٶ󺸵��� true ����.
}
