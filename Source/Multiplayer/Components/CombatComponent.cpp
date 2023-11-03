#include "CombatComponent.h"
#include "Multiplayer/Weapon/Weapon.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Multiplayer/Character/BaseCharacterAnimInstance.h"
#include "Multiplayer/Weapon/Projectile.h"

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
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);//replicated �ǵ��� bAiming�� ���
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly); //replicated �ǵ��� CarriedAmmo�� ���. CarriedAmmo�� ������ �ִ� Client�� ����Ǳ� ������ COND_OwnerOnly ��������� �����Ѵ�. �̷��� �ϸ� Owning Client���� ������ �ǰ� �ٸ� Client�鿡�Դ� ������ ���� �ʴ´�.
	DOREPLIFETIME(UCombatComponent, CombatState); //replicated �ǵ��� CombatState�� ���
	DOREPLIFETIME(UCombatComponent, Grenades); //replicated �ǵ��� Grenades�� ���
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount) // ���� �ݱ�
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		// ���⸦ ������ �ش� ������ ������ Ammo�� ���� Ammo ������ ���Ѵ�. �� �� UpdateCarriedAmmo()�� ������Ʈ �Ѵ�.
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		// ���� ������ �ȵ� ���� && �ش� ���� �Ѿ��� ���� ��� && ���� ���Ⱑ ���������� ���
		Reload(); // ������
	}
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character.IsValid())
	{	
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;// ĳ������ Walk�ִ�ӵ��� BaseWalkSpeed�� ����

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		if (Character->HasAuthority()) // Server
		{
			InitializeCarriedAmmo(); // ���� ���� �� CarriedAmmo ����
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character.IsValid() && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult); // Crosshair���� LineTrace�� ��� HitResult ���� ������Ʈ�Ѵ�.
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime); // HUDcrosshair�� ����
		InterpFOV(DeltaTime); // Aiming O, X ���ο� ���� FOV ����
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire() && EquippedWeapon)
	{
		bCanFire = false;
		// TickComponent()�Լ� ���� TraceUnderCrosshairs()�Լ��� ����Ͽ� Crosshair���� ������ �浹��ġ�� �� ƽ �����Ѵ�.
		// ServerFire()�� �浹��ġ(=HitResult.ImpactPoint=HitTarget)�� �����ش�. MulticastFire_Implementation() ���� EquippedWeapon->Fire(TraceHitTarget)�� ������ �� TraceHitTarget�� �Ʒ��� HitResult.ImpactPoint ���̴�. 
		ServerFire(HitTarget); // Server RPC �� �߻� �Լ�
		LocalFire(HitTarget);

		if (IsValid(EquippedWeapon))
		{
			CrosshairShootingFactor = 0.75f; // �߻� �ÿ��� Crosshair�� ���������� Ư�������� ���ƿ����� ����
		}

		StartFireTimer(); // Automatic Fire Ÿ�̸� �ڵ鷯 ����
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;

	// Ÿ�̸� ����
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;

	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic) //�߻��ư ������ �ְ� ������ ���Ⱑ �ڵ� �߻繫����
	{
		Fire();
	}

	ReloadEmptyWeapon(); // �Ѿ��� ������� Ȯ���ϰ� ���� ����� �� ������
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget) // Server RPC �� �߻� �Լ�
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character.IsValid() && Character->IsLocallyControlled() && Character->HasAuthority() == false) return;

	LocalFire(TraceHitTarget);
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return; // ����ó��. ���� ���Ⱑ ���ٸ� return

	if (Character.IsValid() && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun) // Shotgun
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}

	if (Character.IsValid() && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming); // �߻� ��Ÿ�� Play
		EquippedWeapon->Fire(TraceHitTarget); // ���� ���� �߻�, HitTarget(=TraceHitResult.ImpactPoint <-- HitResult��)
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip) // Server
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	// Primary ����� �ְ�, Secondary ����� ���� ���
	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr) 
	{	
		EquipSecondaryWeapon(WeaponToEquip); // Secondary ���� ����
	}
	else // Primary ���Ⱑ ���� ���
	{	
		EquipPrimaryWeapon(WeaponToEquip); // Primary ���� ����
	}

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;//�������� �� bOrientRotationMovement ���ش�.
	Character->bUseControllerRotationYaw = true;//���콺 �¿�ȸ�� �� ĳ���Ͱ� ȸ���ϸ� ����ؼ� ������ �ٶ󺸵��� true ����.
}

void UCombatComponent::SwapWeapons() // ���� ��ü
{
	if (CombatState != ECombatState::ECS_Unoccupied) return; // �������̳� ����ź ���� ���� ���ⱳü �� �ǵ��� ����ó��

	TWeakObjectPtr<AWeapon> TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon.Get();

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); // �ٲ� ���������� ���¸� EWS_Equipped�� ����.
	AttachActorToRightHand(EquippedWeapon); // �ٲ� �������⸦ ������ ���Ͽ� �ٿ���
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary); // �ٲ� Secondary������ ���¸� EWS_EquippedSecondary�� ����.
	AttachActorToBackpack(SecondaryWeapon); // �ٲ� Secondary ���⸦ �� ���Ͽ� �ٿ���
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;

	DropEquippedWeapon(); // ���⸦ �������̸� �������� ���� ����߸���
	EquippedWeapon = WeaponToEquip; // WeaponToEquip�� ���� ��������� ����
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);//���� ���¸� Equipped(=����)���� ����

	AttachActorToRightHand(EquippedWeapon); // Actor(= ����) ������ ���Ͽ� ���̱�
	EquippedWeapon->SetOwner(Character.Get()); // ������ Owner�� Character�� ����
	EquippedWeapon->SetHUDAmmo(); // HUD�� �Ѿ� �� ������Ʈ

	UpdateCarriedAmmo(); // ���� �������� źâ�� �ִ� �Ѿ� �� ������Ʈ
	PlayEquipWeaponSound(WeaponToEquip); // ���� ���� ���� ���
	ReloadEmptyWeapon(); // �Ѿ��� ������� Ȯ���ϰ� ���� ����� �� ������
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;

	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(WeaponToEquip); // ���⸦ �賶��ġ�� ���δ�.
	PlayEquipWeaponSound(WeaponToEquip); // ���� ���� ���� ���
	
	SecondaryWeapon->SetOwner(Character.Get()); // Secondary ������ Owner�� ĳ���ͷ� ����
}

void UCombatComponent::DropEquippedWeapon() // �������� ���� ����߸��� �Լ�
{
	if (IsValid(EquippedWeapon)) // �̹� ���� ���� ���̶��
	{
		EquippedWeapon->Dropped(); // ���� ���� ���⸦ ����߸���.
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach) // Actor(=����) ������ ���Ͽ� ���̱�
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));// ������ ������ ����
	if (IsValid(HandSocket)) // �ش� ������ �����ϸ�
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh()); // ���⸦ �ش� ���Ͽ� �ٿ��ش�.
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon == nullptr) return;

	bool bUsePistolSocket =
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun;

	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);// ������ ������ ����
	if (IsValid(HandSocket)) // �ش� ������ �����ϸ�
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh()); // ���⸦ �ش� ���Ͽ� �ٿ��ش�.
	}
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;

	const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(FName("BackpackSocket"));
	const USkeletalMeshSocket* RifleSocket = Character->GetMesh()->GetSocketByName(FName("Rifle_AR4_Holster"));
	const USkeletalMeshSocket* RocketLauncherSocket = Character->GetMesh()->GetSocketByName(FName("Rifle_AK47_Holster"));

	AWeapon* WeaponToEquipAtBack = Cast<AWeapon>(ActorToAttach);
	if(IsValid(WeaponToEquipAtBack))
	{
		switch (WeaponToEquipAtBack->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			RifleSocket->AttachActor(ActorToAttach, Character->GetMesh()); // AcotrToAttach(=����)�� �ش� ���Ͽ� �ٿ��ش�.
			break;
		case EWeaponType::EWT_RocketLauncher:
			RocketLauncherSocket->AttachActor(ActorToAttach, Character->GetMesh());
			break;
		case EWeaponType::EWT_Pistol:
			RifleSocket->AttachActor(ActorToAttach, Character->GetMesh());
			break;
		case EWeaponType::EWT_SubmachineGun:
			RifleSocket->AttachActor(ActorToAttach, Character->GetMesh());
			break;
		case EWeaponType::EWT_Shotgun:
			RocketLauncherSocket->AttachActor(ActorToAttach, Character->GetMesh());
			break;
		case EWeaponType::EWT_SniperRifle:
			RocketLauncherSocket->AttachActor(ActorToAttach, Character->GetMesh());
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			RifleSocket->AttachActor(ActorToAttach, Character->GetMesh());
			break;
		}
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return; // ����ó��. ���� ���Ⱑ ���ٸ� return

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) // CarriedAmmoMap�� ����� ����Ÿ���� �ִٸ�
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; // ���� �������� źâ�� �ִ� �Ѿ� ��(=CarriedAmmo)�� CarriedAmmoMap���� �ش� ������ ������ ã�� ����.
	}

	Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
	if (Controller.IsValid())
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo); // HUD�� �ִ� �Ѿ� �� ������Ʈ
	}
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip) // ���� ���� ���� ���
{
	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponToEquip->EquipSound, Character->GetActorLocation()); // �������� ���带 ĳ���� ��ġ���� ���
	}
}

void UCombatComponent::ReloadEmptyWeapon() // �Ѿ��� ������� Ȯ���ϰ� ���� ����� �� ������
{
	if (EquippedWeapon->IsEmpty()) // �߻��� �� �ִ� �Ѿ��� �� �������� 0 ���϶��
	{
		Reload(); // ������
	}
}

void UCombatComponent::Reload()
{
	// CarriedAmmo�� 0���� ū�� Ȯ��. 0���� ������ ������ �� �ʿ�X. CarriedAmmo�� �� ���� �ʾҴ��� Ȯ��
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading && EquippedWeapon && EquippedWeapon->IsFull() == false) 
	{
		ServerReload(); // Server RPC ȣ��.
	}
}

void UCombatComponent::ServerReload_Implementation() // Server RPC, �� �Լ��� ȣ��Ǹ� Server�̵� Client�̵� ���������� ����ȴ�. 
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_Reloading; // CombatState�� ������ ���·� ����
	HandleReload();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;

	if (Character->HasAuthority()) // Server
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues(); // �Ѿ� �� ������Ʈ
	}

	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character.IsValid() && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	// Reload ��Ÿ�ֿ��� ShotgunEnd �������� ����
	TWeakObjectPtr<UAnimInstance> AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance.IsValid() && Character->GetReloadMontage())
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
	if (Controller.IsValid())
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	EquippedWeapon->AddAmmo(-ReloadAmount);
}

void UCombatComponent::UpdateShotgunAmmoValues() // Shotgun �Ѿ� ������Ʈ
{
	if (Character == nullptr || EquippedWeapon == nullptr) return; // ����ó��

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
	if (Controller.IsValid())
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(-1);
	bCanFire = true;
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0) 
	{
		JumpToShotgunEnd(); // Reload ��Ÿ�ֿ��� ShotgunEnd �������� ����
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades(); // ����ź �� HUD�� ������Ʈ �ϱ�
}

void UCombatComponent::OnRep_CombatState() // Client
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_ThrowingGrenade:
		// LocallyControlled�� �̹� ��Ÿ�� ����� �Ͼ�� �ߺ����� ��Ÿ�� ����� ��Ű�� �� �ǹǷ� if üũ 
		if (Character.IsValid() && Character->IsLocallyControlled() == false)
		{
			Character->PlayThrowGrenadeMontage();
		}
		break;
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo(); // ���� �� �ִ� �Ѿ� �� = ������ ���Ⱑ ���� �� �ִ� źâ �ִ� �Ѿ� �� - ���� �Ѿ� ��

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; // ������ ���� źâ�� �ִ� �Ѿ� ��. TMap<EWeaponType, int32> CarriedAmmoMap�� �� ã��. 
		int32 Least = FMath::Min(RoomInMag, AmountCarried);

		return FMath::Clamp(RoomInMag, 0, Least); // RoomInMag�� �������� ������ �ʴ´�. ������ ������ ���ų� �ڵ� �Ǽ��Ͽ� ������ ������ ��Ȳ�� ����ó���ϱ� ���� Clamp ���
	}

	return 0;
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage(); // ������ ��Ÿ�� ���
}

void UCombatComponent::ThrowGrenade() // Client
{
	if (Grenades == 0) return; // ����ó��. ����ź ���� 0�̸� ���� �� �����Ƿ� ����.
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character.IsValid())
	{
		Character->PlayThrowGrenadeMontage(); // ����ź ��ô ��Ÿ�� ���
		AttachActorToLeftHand(EquippedWeapon); // ���� �޼� ���Ͽ� ���̱�
		ShowAttachedGrenade(true); // ����ź �Ž� ���̰� �ϱ�
	}
	if (Character.IsValid() && Character->HasAuthority() == false) // Client
	{
		ServerThrowGrenade(); // Client���� ��Ÿ�ְ� ����� �� Server�� �˸�
	}
	if (Character.IsValid() && Character->HasAuthority()) // Server
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation() // Server
{
	if (Grenades == 0) return; // ����ó��. ����ź ���� 0�̸� ���� �� �����Ƿ� ����.

	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character.IsValid())
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon); // ���� �޼� ���Ͽ� ���̱�
		ShowAttachedGrenade(true); // ����ź �Ž� ���̰� �ϱ�
	}

	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades); // ���� �� ����ź �� -1 ���ֱ�
	UpdateHUDGrenades(); // ����ź �� HUD ������Ʈ
}

void UCombatComponent::UpdateHUDGrenades()
{
	Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
	if (Controller.IsValid())
	{
		Controller->SetHUDGrenades(Grenades); // ���� ����ź ���� �Ű������� �ѱ�� HUD ������Ʈ
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character.IsValid() && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade); // bShowGrenade�� �������� �������� ����ź�� ������ ���� ����
	}
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false); // ����ź�� ��ô�Ǹ� ĳ���� �տ� ���� ����ź�� �� ���̵��� ���ش�.

	if (Character.IsValid() && Character->IsLocallyControlled()) // Client���
	{
		ServerLaunchGrenade(HitTarget);
	}
}

// Target ��ġ�� �Ű������� �Ѱܹ޾� ������ ���⿡ ����Ѵ�. 
void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target) // Server RPC
{
	if (Character.IsValid() && GrenadeClass && Character->GetAttachedGrenade()) // Server�� ����
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation(); // ĳ���Ϳ� ���� ����ź ��ġ
		FVector ToTarget = Target - StartingLocation; // Target�� ���ϴ� ����
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character.Get();
		SpawnParams.Instigator = Character.Get(); // Projetile.h.cpp�� ExplodeDamage()���� GetInstigator()�� �Ѵ�. ���⼭ Instigator�� �������� ������ ������ �����ʾ� ������ �ȴ�.
		TWeakObjectPtr<UWorld> World = GetWorld();

		if (World.IsValid())
		{
			World->SpawnActor<AProjectile>(GrenadeClass, StartingLocation, ToTarget.Rotation(), SpawnParams);
		}
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon); // ���� ������ ���Ͽ� ���̱�
}

void UCombatComponent::OnRep_EquippedWeapon() // Client
{
	if (EquippedWeapon && Character.IsValid()) // �������� ��
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);//���� ���¸� Equipped(=����)���� ����
		AttachActorToRightHand(EquippedWeapon); // ���� ������ ���Ͽ� ���̱�
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;//�������� �� bOrientRotationMovement ���ش�.
		Character->bUseControllerRotationYaw = true;//���콺 �¿�ȸ�� �� ĳ���Ͱ� ȸ���ϸ� ����ؼ� ������ �ٶ󺸵��� true ����.
		PlayEquipWeaponSound(EquippedWeapon); // ���� ���� ���� ���
		EquippedWeapon->EnableCustomDepth(false); // ���� �ܰ��� ȿ�� false
		EquippedWeapon->SetHUDAmmo(); // �Ѿ� HUD ������Ʈ
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (IsValid(SecondaryWeapon) && Character.IsValid())
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

		AttachActorToBackpack(SecondaryWeapon); // Secondary ���⸦ � ���δ�
		PlayEquipWeaponSound(EquippedWeapon); // ���� ���� ���
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

		if (Character.IsValid())
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.0f);
			//DrawDebugSphere(GetWorld(), Start, 16.0f, 12, FColor::Red, false);//������
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH; //TRACE_LENGTH�� ���� ������ 80000.0f

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (TraceHitResult.bBlockingHit == false) //�ϴ� ���� �浹�Ұ� ���°��� ��� ���
		{
			TraceHitResult.ImpactPoint = End; //�浹�ϴ°� ���ٸ� End ���� ImpactPoint������ ����.
		}

		//**Crosshair �� �����ϱ�
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UICrosshair>())
		{
			HUDPackage.CrosshairColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;


	Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;

	if (Controller.IsValid()) // (MainPlayer)Conroller�� �ִٸ�
	{
		HUD = HUD == nullptr ? Cast<AMainHUD>(Controller->GetHUD()) : HUD;

		if (HUD.IsValid())
		{
			if (IsValid(EquippedWeapon)) // ������ ���Ⱑ �ִٸ� Crosshair ����
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
			}
			else // ������ ���Ⱑ ���ٸ� Crosshair = nullptr ����
			{
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
				HUDPackage.CrosshairTop = nullptr;
			}

			//** Crosshair ������ ���� ����ϱ�
			// [0, 600] -> [0, 1]
			FVector2D WalkSpeedRange(0.0f, Character->GetCharacterMovement()->MaxWalkSpeed);// �ȱ� �ӵ�
			FVector2D VelocityMultiplierRange(0.0f, 1.0f);//
			FVector Velocity = Character->GetVelocity(); // ĳ������ �ӵ�
			Velocity.Z = 0.0f; // ĳ������ �ӵ� �� Z���� 0���� ����

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());// Velocity.Size()���� WalkSpeedRange ���� ���� ������ VelocityMultiplierRange ���� ���� ������ ġȯ�Ѵ�.

			if (Character->GetCharacterMovement()->IsFalling()) // ���� O
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else // ���� X, �ٴڿ� ���� ��
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.0f, DeltaTime, 30.0f);
			}

			if (bAiming) // Aiming O
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.0f);
			}
			else // Aiming X
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.0f, DeltaTime, 30.0f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.0f, DeltaTime, 40.0f);

			HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
			//**  **//

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return; // ���� ���� ���� �ƴ϶�� return

	//** ���� ���� ��
	if (bAiming) // Aiming O
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());// FOV ��ȯ
	}
	else // Aiming X
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);// FOV ��ȯ, ZoomInterpSpeed�� �ð����ݵ��� CurrentFOV�� ���� DefaultFOV�� ����
	}

	if (Character.IsValid() && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV); // ĳ���� ī�޶��� FOV���� CurrentFOV������ ����
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return; // ���� ó��

	//������ �����ϴ� ��� ĳ���Ϳ� ��� clinet�� Aiming ��� �� �� �ֵ��� �Ѵ�.
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character.IsValid())
	{
		//ĳ���Ͱ� ����(=Aiming)���̶�� MaxWalkSpeed�� AimWalkSpeed�� �ƴϸ� BaseWalkSpeed�� ����.
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

	//** �������� ���
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming); // SniperScope ������ ���̰� �Ѵ�.
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)//RPC���� _Implementation���� ���
{
	bAiming = bIsAiming;
	if (Character.IsValid())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false; // ������ ���Ⱑ ���ٸ� false ����

	if (EquippedWeapon->IsEmpty() == false && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun) return true;

	return EquippedWeapon->IsEmpty() == false && bCanFire && CombatState == ECombatState::ECS_Unoccupied; // �Ѿ��� ������� �ʾҴٸ�(=�Ѿ��� �ִٸ�) 
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
	if (Controller.IsValid())
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	bool bJumpToShotgunEnd =
		CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon != nullptr &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CarriedAmmo == 0;
	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::InitializeCarriedAmmo() // ���� ���� �� �־����� �� ���� �� ź�� �� ����
{
	// ����� Starting����Ammo�� TMap�� �ѽ����� ���� ���� �� ������ �� �⺻������ ����
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo); 
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo); 
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}
