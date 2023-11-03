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
	PrimaryComponentTick.bCanEverTick = true; // trace를 매 tick 확인하기 위해 true로 설정.

	BaseWalkSpeed = 600.0f;
	AimWalkSpeed = 450.0f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);//replicated된 EquippedWeapon. EquippedWeapon이 변경되면 모든 client에 반영된다.
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);//replicated 되도록 bAiming을 등록
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly); //replicated 되도록 CarriedAmmo을 등록. CarriedAmmo를 가지고 있는 Client만 적용되기 때문에 COND_OwnerOnly 컨디션으로 설정한다. 이렇게 하면 Owning Client에만 적용이 되고 다른 Client들에게는 적용이 되지 않는다.
	DOREPLIFETIME(UCombatComponent, CombatState); //replicated 되도록 CombatState을 등록
	DOREPLIFETIME(UCombatComponent, Grenades); //replicated 되도록 Grenades을 등록
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount) // 무기 줍기
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		// 무기를 주으면 해당 종류의 무기의 Ammo에 주은 Ammo 개수를 더한다. 그 후 UpdateCarriedAmmo()로 업데이트 한다.
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		// 무기 장착이 안된 상태 && 해당 무기 총알이 없는 경우 && 주은 무기가 장착무기인 경우
		Reload(); // 재장전
	}
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character.IsValid())
	{	
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;// 캐릭터의 Walk최대속도를 BaseWalkSpeed로 설정

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		if (Character->HasAuthority()) // Server
		{
			InitializeCarriedAmmo(); // 게임 시작 시 CarriedAmmo 설정
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character.IsValid() && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult); // Crosshair에서 LineTrace를 쏘고 HitResult 값을 업데이트한다.
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime); // HUDcrosshair를 갱신
		InterpFOV(DeltaTime); // Aiming O, X 여부에 따라 FOV 변경
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
		// TickComponent()함수 내의 TraceUnderCrosshairs()함수를 사용하여 Crosshair으로 나가서 충돌위치를 매 틱 갱신한다.
		// ServerFire()에 충돌위치(=HitResult.ImpactPoint=HitTarget)를 보내준다. MulticastFire_Implementation() 내의 EquippedWeapon->Fire(TraceHitTarget)를 실행할 때 TraceHitTarget은 아래의 HitResult.ImpactPoint 값이다. 
		ServerFire(HitTarget); // Server RPC 총 발사 함수
		LocalFire(HitTarget);

		if (IsValid(EquippedWeapon))
		{
			CrosshairShootingFactor = 0.75f; // 발사 시에는 Crosshair의 퍼짐정도가 특정값으로 돌아오도록 설정
		}

		StartFireTimer(); // Automatic Fire 타이머 핸들러 시작
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;

	// 타이머 설정
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
	if (bFireButtonPressed && EquippedWeapon->bAutomatic) //발사버튼 누르고 있고 장착된 무기가 자동 발사무기라면
	{
		Fire();
	}

	ReloadEmptyWeapon(); // 총알이 비었는지 확인하고 만약 비었을 시 재장전
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget) // Server RPC 총 발사 함수
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
	if (EquippedWeapon == nullptr) return; // 예외처리. 장착 무기가 없다면 return

	if (Character.IsValid() && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun) // Shotgun
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}

	if (Character.IsValid() && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming); // 발사 몽타주 Play
		EquippedWeapon->Fire(TraceHitTarget); // 장착 무기 발사, HitTarget(=TraceHitResult.ImpactPoint <-- HitResult값)
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip) // Server
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	// Primary 무기는 있고, Secondary 무기는 없는 경우
	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr) 
	{	
		EquipSecondaryWeapon(WeaponToEquip); // Secondary 무기 장착
	}
	else // Primary 무기가 없는 경우
	{	
		EquipPrimaryWeapon(WeaponToEquip); // Primary 무기 장착
	}

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;//무기장착 시 bOrientRotationMovement 꺼준다.
	Character->bUseControllerRotationYaw = true;//마우스 좌우회전 시 캐릭터가 회전하며 계속해서 정면을 바라보도록 true 설정.
}

void UCombatComponent::SwapWeapons() // 무기 교체
{
	if (CombatState != ECombatState::ECS_Unoccupied) return; // 재장전이나 수류탄 던질 때는 무기교체 안 되도록 예외처리

	TWeakObjectPtr<AWeapon> TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon.Get();

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); // 바뀐 장착무기의 상태를 EWS_Equipped로 설정.
	AttachActorToRightHand(EquippedWeapon); // 바뀐 장착무기를 오른손 소켓에 붙여줌
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary); // 바뀐 Secondary무기의 상태를 EWS_EquippedSecondary로 설정.
	AttachActorToBackpack(SecondaryWeapon); // 바뀐 Secondary 무기를 등 소켓에 붙여줌
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;

	DropEquippedWeapon(); // 무기를 장착중이면 장착중인 무기 떨어뜨리기
	EquippedWeapon = WeaponToEquip; // WeaponToEquip를 현재 장착무기로 설정
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);//무기 상태를 Equipped(=장착)으로 변경

	AttachActorToRightHand(EquippedWeapon); // Actor(= 무기) 오른손 소켓에 붙이기
	EquippedWeapon->SetOwner(Character.Get()); // 무기의 Owner을 Character로 설정
	EquippedWeapon->SetHUDAmmo(); // HUD에 총알 수 업데이트

	UpdateCarriedAmmo(); // 현재 장착무기 탄창의 최대 총알 수 업데이트
	PlayEquipWeaponSound(WeaponToEquip); // 무기 장착 사운드 재생
	ReloadEmptyWeapon(); // 총알이 비었는지 확인하고 만약 비었을 시 재장전
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;

	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(WeaponToEquip); // 무기를 배낭위치에 붙인다.
	PlayEquipWeaponSound(WeaponToEquip); // 무기 장착 사운드 재생
	
	SecondaryWeapon->SetOwner(Character.Get()); // Secondary 무기의 Owner를 캐릭터로 설정
}

void UCombatComponent::DropEquippedWeapon() // 장착중인 무기 떨어뜨리기 함수
{
	if (IsValid(EquippedWeapon)) // 이미 무기 장착 중이라면
	{
		EquippedWeapon->Dropped(); // 장착 중인 무기를 떨어뜨린다.
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach) // Actor(=무기) 오른손 소켓에 붙이기
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));// 소켓을 변수로 저장
	if (IsValid(HandSocket)) // 해당 소켓이 존재하면
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh()); // 무기를 해당 소켓에 붙여준다.
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon == nullptr) return;

	bool bUsePistolSocket =
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun;

	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);// 소켓을 변수로 저장
	if (IsValid(HandSocket)) // 해당 소켓이 존재하면
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh()); // 무기를 해당 소켓에 붙여준다.
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
			RifleSocket->AttachActor(ActorToAttach, Character->GetMesh()); // AcotrToAttach(=무기)를 해당 소켓에 붙여준다.
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
	if (EquippedWeapon == nullptr) return; // 예외처리. 장착 무기가 없다면 return

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) // CarriedAmmoMap에 변경된 무기타입이 있다면
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; // 현재 장착무기 탄창의 최대 총알 수(=CarriedAmmo)를 CarriedAmmoMap에서 해당 무기의 정보를 찾아 설정.
	}

	Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
	if (Controller.IsValid())
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo); // HUD에 최대 총알 수 업데이트
	}
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip) // 무기 장착 사운드 재생
{
	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponToEquip->EquipSound, Character->GetActorLocation()); // 무기장착 사운드를 캐릭터 위치에서 재생
	}
}

void UCombatComponent::ReloadEmptyWeapon() // 총알이 비었는지 확인하고 만약 비었을 시 재장전
{
	if (EquippedWeapon->IsEmpty()) // 발사할 수 있는 총알이 다 떨어져서 0 이하라면
	{
		Reload(); // 재장전
	}
}

void UCombatComponent::Reload()
{
	// CarriedAmmo가 0보다 큰지 확인. 0보다 작은면 재장전 할 필요X. CarriedAmmo가 꽉 차지 않았는지 확인
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading && EquippedWeapon && EquippedWeapon->IsFull() == false) 
	{
		ServerReload(); // Server RPC 호출.
	}
}

void UCombatComponent::ServerReload_Implementation() // Server RPC, 이 함수가 호출되면 Server이든 Client이든 서버에서만 실행된다. 
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_Reloading; // CombatState을 재장전 상태로 변경
	HandleReload();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;

	if (Character->HasAuthority()) // Server
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues(); // 총알 수 업데이트
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
	// Reload 몽타주에서 ShotgunEnd 섹션으로 점프
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

void UCombatComponent::UpdateShotgunAmmoValues() // Shotgun 총알 업데이트
{
	if (Character == nullptr || EquippedWeapon == nullptr) return; // 예외처리

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
		JumpToShotgunEnd(); // Reload 몽타주에서 ShotgunEnd 섹션으로 점프
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades(); // 수류탄 수 HUD에 업데이트 하기
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
		// LocallyControlled면 이미 몽타주 재생이 일어나니 중복으로 몽타주 재생을 시키면 안 되므로 if 체크 
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
	
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo(); // 넣을 수 있는 총알 수 = 장착된 무기가 가질 수 있는 탄창 최대 총알 수 - 현재 총알 수

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; // 장착된 무기 탄창의 최대 총알 수. TMap<EWeaponType, int32> CarriedAmmoMap로 값 찾기. 
		int32 Least = FMath::Min(RoomInMag, AmountCarried);

		return FMath::Clamp(RoomInMag, 0, Least); // RoomInMag가 음수값이 나오지 않는다. 하지만 오류가 나거나 코드 실수하여 음수가 나오는 상황을 예외처리하기 위해 Clamp 사용
	}

	return 0;
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage(); // 재장전 몽타주 재생
}

void UCombatComponent::ThrowGrenade() // Client
{
	if (Grenades == 0) return; // 예외처리. 수류탄 수가 0이면 던질 수 없으므로 리턴.
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character.IsValid())
	{
		Character->PlayThrowGrenadeMontage(); // 수류탄 투척 몽타주 재생
		AttachActorToLeftHand(EquippedWeapon); // 무기 왼손 소켓에 붙이기
		ShowAttachedGrenade(true); // 수류탄 매쉬 보이게 하기
	}
	if (Character.IsValid() && Character->HasAuthority() == false) // Client
	{
		ServerThrowGrenade(); // Client에서 몽타주가 재생된 걸 Server에 알림
	}
	if (Character.IsValid() && Character->HasAuthority()) // Server
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation() // Server
{
	if (Grenades == 0) return; // 예외처리. 수류탄 수가 0이면 던질 수 없으므로 리턴.

	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character.IsValid())
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon); // 무기 왼손 소켓에 붙이기
		ShowAttachedGrenade(true); // 수류탄 매쉬 보이게 하기
	}

	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades); // 던진 후 수류탄 수 -1 해주기
	UpdateHUDGrenades(); // 수류탄 수 HUD 업데이트
}

void UCombatComponent::UpdateHUDGrenades()
{
	Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
	if (Controller.IsValid())
	{
		Controller->SetHUDGrenades(Grenades); // 현재 수류탄 수를 매개변수로 넘기고 HUD 업데이트
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character.IsValid() && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade); // bShowGrenade의 변수값을 기준으로 수류탄이 보일지 말지 결정
	}
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false); // 수류탄이 투척되면 캐릭터 손에 붙은 수류탄은 안 보이도록 꺼준다.

	if (Character.IsValid() && Character->IsLocallyControlled()) // Client라면
	{
		ServerLaunchGrenade(HitTarget);
	}
}

// Target 위치를 매개변수로 넘겨받아 던지는 방향에 사용한다. 
void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target) // Server RPC
{
	if (Character.IsValid() && GrenadeClass && Character->GetAttachedGrenade()) // Server일 때만
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation(); // 캐릭터에 붙은 수류탄 위치
		FVector ToTarget = Target - StartingLocation; // Target을 향하는 벡터
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character.Get();
		SpawnParams.Instigator = Character.Get(); // Projetile.h.cpp의 ExplodeDamage()에서 GetInstigator()를 한다. 여기서 Instigator를 설정하지 않으면 정보를 주지않아 문제가 된다.
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
	AttachActorToRightHand(EquippedWeapon); // 무기 오른손 소켓에 붙이기
}

void UCombatComponent::OnRep_EquippedWeapon() // Client
{
	if (EquippedWeapon && Character.IsValid()) // 무기장착 시
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);//무기 상태를 Equipped(=장착)으로 변경
		AttachActorToRightHand(EquippedWeapon); // 무기 오른손 소켓에 붙이기
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;//무기장착 시 bOrientRotationMovement 꺼준다.
		Character->bUseControllerRotationYaw = true;//마우스 좌우회전 시 캐릭터가 회전하며 계속해서 정면을 바라보도록 true 설정.
		PlayEquipWeaponSound(EquippedWeapon); // 무기 장착 사운드 재생
		EquippedWeapon->EnableCustomDepth(false); // 무기 외곽선 효과 false
		EquippedWeapon->SetHUDAmmo(); // 총알 HUD 업데이트
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (IsValid(SecondaryWeapon) && Character.IsValid())
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

		AttachActorToBackpack(SecondaryWeapon); // Secondary 무기를 등에 붙인다
		PlayEquipWeaponSound(EquippedWeapon); // 장착 사운드 재생
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f); //화면 중앙
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	); // DeprojectScreenToWorld 성공하면 true, 실패하면 false

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		if (Character.IsValid())
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.0f);
			//DrawDebugSphere(GetWorld(), Start, 16.0f, 12, FColor::Red, false);//디버깅용
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH; //TRACE_LENGTH는 내가 설정한 80000.0f

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (TraceHitResult.bBlockingHit == false) //하늘 같이 충돌할게 없는곳에 쏘는 경우
		{
			TraceHitResult.ImpactPoint = End; //충돌하는게 없다면 End 값을 ImpactPoint값으로 설정.
		}

		//**Crosshair 색 설정하기
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

	if (Controller.IsValid()) // (MainPlayer)Conroller가 있다면
	{
		HUD = HUD == nullptr ? Cast<AMainHUD>(Controller->GetHUD()) : HUD;

		if (HUD.IsValid())
		{
			if (IsValid(EquippedWeapon)) // 장착된 무기가 있다면 Crosshair 세팅
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
			}
			else // 장착된 무기가 없다면 Crosshair = nullptr 세팅
			{
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
				HUDPackage.CrosshairTop = nullptr;
			}

			//** Crosshair 퍼지는 정도 계산하기
			// [0, 600] -> [0, 1]
			FVector2D WalkSpeedRange(0.0f, Character->GetCharacterMovement()->MaxWalkSpeed);// 걷기 속도
			FVector2D VelocityMultiplierRange(0.0f, 1.0f);//
			FVector Velocity = Character->GetVelocity(); // 캐릭터의 속도
			Velocity.Z = 0.0f; // 캐릭터의 속도 중 Z값은 0으로 설정

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());// Velocity.Size()값을 WalkSpeedRange 범위 내의 값에서 VelocityMultiplierRange 범위 내의 값으로 치환한다.

			if (Character->GetCharacterMovement()->IsFalling()) // 공중 O
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else // 공중 X, 바닥에 있을 때
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
	if (EquippedWeapon == nullptr) return; // 무기 장착 중이 아니라면 return

	//** 무기 장착 중
	if (bAiming) // Aiming O
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());// FOV 전환
	}
	else // Aiming X
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);// FOV 전환, ZoomInterpSpeed의 시간간격동안 CurrentFOV의 값이 DefaultFOV로 변경
	}

	if (Character.IsValid() && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV); // 캐릭터 카메라의 FOV값을 CurrentFOV값으로 변경
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return; // 예외 처리

	//서버가 관리하는 모든 캐릭터에 모든 clinet가 Aiming 포즈를 볼 수 있도록 한다.
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character.IsValid())
	{
		//캐릭터가 조준(=Aiming)중이라면 MaxWalkSpeed를 AimWalkSpeed로 아니면 BaseWalkSpeed로 설정.
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

	//** 저격총인 경우
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming); // SniperScope 위젯을 보이게 한다.
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)//RPC들은 _Implementation버젼 사용
{
	bAiming = bIsAiming;
	if (Character.IsValid())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false; // 장착된 무기가 없다면 false 리턴

	if (EquippedWeapon->IsEmpty() == false && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun) return true;

	return EquippedWeapon->IsEmpty() == false && bCanFire && CombatState == ECombatState::ECS_Unoccupied; // 총알이 비어있지 않았다면(=총알이 있다면) 
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

void UCombatComponent::InitializeCarriedAmmo() // 게임 시작 시 주어지는 각 무기 당 탄알 수 설정
{
	// 무기과 Starting무기Ammo을 TMap의 한쌍으로 게임 시작 시 가지게 될 기본값으로 설정
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo); 
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo); 
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}
