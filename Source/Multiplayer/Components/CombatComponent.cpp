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
	DOREPLIFETIME(UCombatComponent, bAiming);//replicated 되도록 bAiming을 등록
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly); //replicated 되도록 CarriedAmmo을 등록. CarriedAmmo를 가지고 있는 Client만 적용되기 때문에 COND_OwnerOnly 컨디션으로 설정한다. 이렇게 하면 Owning Client에만 적용이 되고 다른 Client들에게는 적용이 되지 않는다.
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
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget) // Server RPC 총 발사 함수
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return; //장착 무기가 없다면 return

	if (Character.IsValid())
	{
		Character->PlayFireMontage(bAiming); // 발사 몽타주 Play
		EquippedWeapon->Fire(TraceHitTarget); // 장착 무기 발사, HitTarget(=TraceHitResult.ImpactPoint <-- HitResult값)
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (IsValid(EquippedWeapon)) // 이미 무기 장착 중이라면
	{
		EquippedWeapon->Dropped(); // 장착 중인 무기를 떨어뜨린다.
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);//무기 상태를 Equipped(=장착)으로 변경
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));//소켓을 변수로 저장
	if (IsValid(HandSocket)) // 해당 소켓이 존재하면
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh()); //무기를 해당 소켓에 붙여준다.
	}
	EquippedWeapon->SetOwner(Character.Get()); // 무기의 Owner을 Character로 설정
	EquippedWeapon->SetHUDAmmo(); // HUD에 총알 수 업데이트

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) // CarriedAmmoMap에 변경된 무기타입이 있다면
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; // 현재 장착무기 탄창의 최대 총알 수(=CarriedAmmo)를 CarriedAmmoMap에서 해당 무기의 정보를 찾아 설정.
	}

	Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
	if (Controller.IsValid())
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo); // HUD에 최대 총알 수 업데이트
	}

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;//무기장착 시 bOrientRotationMovement 꺼준다.
	Character->bUseControllerRotationYaw = true;//마우스 좌우회전 시 캐릭터가 회전하며 계속해서 정면을 바라보도록 true 설정.
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0) // 0보다 큰지 확인. 0보다 작은면 재장전 할 필요X
	{
		ServerReload(); // Server RPC 호출.
	}
}

void UCombatComponent::ServerReload_Implementation() // Server RPC, 이 함수가 호출되면 Server이든 Client이든 서버에서만 실행된다. 
{
	if (Character.IsValid() == false) return;

	Character->PlayReloadMontage(); // 재장전 몽타주 재생
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character.IsValid()) //무기장착 시
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);//무기 상태를 Equipped(=장착)으로 변경
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));//소켓을 변수로 저장
		if (IsValid(HandSocket)) // 해당 소켓이 존재하면
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh()); //무기를 해당 소켓에 붙여준다.
		}
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;//무기장착 시 bOrientRotationMovement 꺼준다.
		Character->bUseControllerRotationYaw = true;//마우스 좌우회전 시 캐릭터가 회전하며 계속해서 정면을 바라보도록 true 설정.
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
	//서버가 관리하는 모든 캐릭터에 모든 clinet가 Aiming 포즈를 볼 수 있도록 한다.
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character.IsValid())
	{
		//캐릭터가 조준(=Aiming)중이라면 MaxWalkSpeed를 AimWalkSpeed로 아니면 BaseWalkSpeed로 설정.
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
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

	return !EquippedWeapon->IsEmpty() || !bCanFire; // 총알이 비어있지 않았다면(=총알이 있다면) 
}

void UCombatComponent::OnRep_CarriedAmmo()
{
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo); // AssaultRifle과 StartingARAmmo 한쌍을 게임 시작 시 기본값으로 설정
}
