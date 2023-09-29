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
	PrimaryComponentTick.bCanEverTick = true; // trace를 매 tick 확인하기 위해 true로 설정.

	BaseWalkSpeed = 600.0f;
	AimWalkSpeed = 450.0f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);//replicated된 EquippedWeapon. EquippedWeapon이 변경되면 모든 client에 반영된다.
	DOREPLIFETIME(UCombatComponent, bAiming);//replicated 되도록 bAiming을 등록
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{	// 캐릭터의 Walk최대속도를 BaseWalkSpeed로 설정.
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	//서버가 관리하는 모든 캐릭터에 모든 clinet가 Aiming 포즈를 볼 수 있도록 한다.
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		//캐릭터가 조준(=Aiming)중이라면 MaxWalkSpeed를 AimWalkSpeed로 아니면 BaseWalkSpeed로 설정.
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)//RPC들은 _Implementation버젼 사용
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character) //무기장착 시
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;//무기장착 시 bOrientRotationMovement 꺼준다.
		Character->bUseControllerRotationYaw = true;//마우스 좌우회전 시 캐릭터가 회전하며 계속해서 정면을 바라보도록 true 설정.
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		ServerFire(); // Server RPC 총 발사 함수
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
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH; //TRACE_LENGTH는 내가 설정한 80000.0f

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (TraceHitResult.bBlockingHit == false)
		{
			TraceHitResult.ImpactPoint = End; //충돌하는게 없다면 End 값을 ImpactPoint값으로 설정. 
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

void UCombatComponent::ServerFire_Implementation() // Server RPC 총 발사 함수
{
	MulticastFire();
}

void UCombatComponent::MulticastFire_Implementation()
{
	if (EquippedWeapon == nullptr) return; //장착 무기가 없다면 return

	if (Character)
	{
		Character->PlayFireMontage(bAiming); // 발사 몽타주 Play
		EquippedWeapon->Fire(); // 장착 무기 발사
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
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);//무기 상태를 Equipped(=장착)으로 변경
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));//소켓을 변수로 저장
	if (IsValid(HandSocket)) // 해당 소켓이 존재하면
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh()); //무기를 해당 소켓에 붙여준다.
	}
	EquippedWeapon->SetOwner(Character); // 무기의 Owner을 Character로 설정
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;//무기장착 시 bOrientRotationMovement 꺼준다.
	Character->bUseControllerRotationYaw = true;//마우스 좌우회전 시 캐릭터가 회전하며 계속해서 정면을 바라보도록 true 설정.
}
