#include "CombatComponent.h"
#include "Multiplayer/Weapon/Weapon.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // trace�� �� tick Ȯ���ϱ� ���� true�� ����.

	BaseWalkSpeed = 600.0f;
	AimWalkSpeed = 450.0f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);//replicated�� EquippedWeapon. EquippedWeapon�� ����Ǹ� ��� client�� �ݿ��ȴ�.
	DOREPLIFETIME(UCombatComponent, bAiming);//replicated �ǵ��� bAiming�� ���
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

	if (bFireButtonPressed)
	{
		ServerFire(); // Server RPC �� �߻� �Լ�
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f); //ȭ�� �߾�
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	); // DeprojectScreenToWorld �����ϸ� true, �����ϸ� false

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH; //TRACE_LENGTH�� ���� ������ 80000.0f

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (TraceHitResult.bBlockingHit == false)
		{
			TraceHitResult.ImpactPoint = End; //�浹�ϴ°� ���ٸ� End ���� ImpactPoint������ ����. 
		}
		else
		{
			DrawDebugSphere(
				GetWorld(),
				TraceHitResult.ImpactPoint,
				12.0f,
				12,
				FColor::Red
			);
		}
	}
}

void UCombatComponent::ServerFire_Implementation() // Server RPC �� �߻� �Լ�
{
	MulticastFire();
}

void UCombatComponent::MulticastFire_Implementation()
{
	if (EquippedWeapon == nullptr) return; //���� ���Ⱑ ���ٸ� return

	if (Character)
	{
		Character->PlayFireMontage(bAiming); // �߻� ��Ÿ�� Play
		EquippedWeapon->Fire(); // ���� ���� �߻�
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult HitResult;
	TraceUnderCrosshairs(HitResult);
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
