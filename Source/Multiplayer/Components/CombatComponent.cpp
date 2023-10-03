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

	if (Character.IsValid())
	{	
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;// ĳ������ Walk�ִ�ӵ��� BaseWalkSpeed�� ����

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character.IsValid() && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime); // HUDcrosshair�� ����
		InterpFOV(DeltaTime); // Aiming O, X ���ο� ���� FOV ����
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	//������ �����ϴ� ��� ĳ���Ϳ� ��� clinet�� Aiming ��� �� �� �ֵ��� �Ѵ�.
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character.IsValid())
	{
		//ĳ���Ͱ� ����(=Aiming)���̶�� MaxWalkSpeed�� AimWalkSpeed�� �ƴϸ� BaseWalkSpeed�� ����.
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
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

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character.Get()) //�������� ��
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
		// TickComponent()�Լ��� �ƴ� ���� ��ġ�� TraceUnderCrosshairs()�Լ��� ��
		// ServerFire()�� �浹��ġ(=HitResult.ImpactPoint)�� �����ش�. MulticastFire_Implementation() ���� EquippedWeapon->Fire(TraceHitTarget)�� ������ �� TraceHitTarget�� �Ʒ��� HitResult.ImpactPoint ���̴�. 
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult); // Crosshair���� LineTrace�� ��� HitResult ���� ������Ʈ�Ѵ�.

		ServerFire(HitResult.ImpactPoint); // Server RPC �� �߻� �Լ�

		if (IsValid(EquippedWeapon))
		{
			CrosshairShootingFactor = 0.75f; // �߻� �ÿ��� Crosshair�� ���������� Ư�������� ���ƿ����� ����
		}
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

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget) // Server RPC �� �߻� �Լ�
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return; //���� ���Ⱑ ���ٸ� return

	if (Character.IsValid())
	{
		Character->PlayFireMontage(bAiming); // �߻� ��Ÿ�� Play
		EquippedWeapon->Fire(TraceHitTarget); // ���� ���� �߻�, HitTarget(=TraceHitResult.ImpactPoint <-- HitResult��)
	}
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
	EquippedWeapon->SetOwner(Character.Get()); // ������ Owner�� Character�� ����
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;//�������� �� bOrientRotationMovement ���ش�.
	Character->bUseControllerRotationYaw = true;//���콺 �¿�ȸ�� �� ĳ���Ͱ� ȸ���ϸ� ����ؼ� ������ �ٶ󺸵��� true ����.
}
