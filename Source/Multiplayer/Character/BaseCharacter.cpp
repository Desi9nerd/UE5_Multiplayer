#include "BaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Multiplayer/Weapon/Weapon.h"
#include "Multiplayer/Components/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(GetMesh());//Mesh �Ʒ� �׸����� ���δ�.
	CameraSpringArm->TargetArmLength = 600.0f;
	CameraSpringArm->bUsePawnControlRotation = true;//true: ���콺�� ������ �� controller�� ���� SpringArm�� ȸ����ų �� �ִ�

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false; //Unequipped�� ���������� ���콺�� ������ ȭ�鸸 ���ư��� �ʰ� �÷��̾�� ���� ���ư����� false ����.
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true); //Combat�� Replicated ������Ʈ�� ������ش�.

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); //ĸ���� ī�޶� ������ �浹�� ����.
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); //�Ž��� ī�޶� ������ �浹�� ����.

	TurningInPlace = ETurningInPlace::ETIP_NotTurning; //TurningInPlace �⺻�� ����.
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABaseCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABaseCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABaseCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABaseCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABaseCharacter::AimButtonReleased);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABaseCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABaseCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABaseCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABaseCharacter::LookUp);
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABaseCharacter, OverlappingWeapon, COND_OwnerOnly);//OwnerOnly: �ش� ĳ���͸� ������ �ִ� Client�� ����
}

void ABaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (IsValid(Combat))
	{
		Combat->Character = this;
	}
}

void ABaseCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.0f)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);//ĳ������ Yaw���� �����Ϳ��� -90���� ���ȱ� ������ Controller->GetControlRotation().Yaw�� �ٲ� �� ���.
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));//������ Yaw�� X������ FVector
		AddMovementInput(Direction, Value);//�� �������� Value��ŭ �̵�
	}
}

void ABaseCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.0f)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));//������ Yaw�� Y������ FVector
		AddMovementInput(Direction, Value);//������ �������� Value��ŭ �̵�
	}
}

void ABaseCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABaseCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABaseCharacter::EquipButtonPressed()
{
	if (Combat)
	{
		if (HasAuthority()) //���� ����. Server���� validate�ϴ� HasAuthority()
		{
			Combat->EquipWeapon(OverlappingWeapon); //���⸦ �־ ĳ���Ϳ� ������Ų��
		}
		else //Ŭ���̾�Ʈ ����. Authority�� ���� ���, RPC�� ���� ServerEquipButtonPressed_Implementation()���� �������� ����
		{
			ServerEquipButtonPressed(); //���� ����
		}
	}
}

void ABaseCharacter::ServerEquipButtonPressed_Implementation()
{
	// ������ ���� ���� �����ϴ� ���
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);//���⸦ �ݰ� ĳ���Ϳ� ������Ų��
	}
}

void ABaseCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch(); // �𸮾� �����Լ� Crouch()���
	}
}

void ABaseCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABaseCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABaseCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.0f && bIsInAir == false) // standing O, ���� X
	{
		FRotator CurrentAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation); // ȸ����(=���⼭�� Yaw��) Normalize
		AO_Yaw = DeltaAimRotation.Yaw; // Yaw�� ����.
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;// ȸ�������ʴ� �������¶�� InterpAO_Yaw=AO_Yaw ����
		}
		bUseControllerRotationYaw = true; //���� �� ���콺 ȸ������ ī�޶� ȸ���� �������� �ʵ��� true ����.
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.0f || bIsInAir) // running O �Ǵ� ���� O
	{
		StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		AO_Yaw = 0.0f;
		bUseControllerRotationYaw = true; // ���콺 ȸ�� �� ĳ���Ͱ� ���� ������ true ����
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.0f && IsLocallyControlled() == false)// 90���� �Ѱ�, ȣ��Ʈ���
	{
		// AO_Pitch�� ������ �����Ѵ�.from [270, 360) to [-90, 0)
		FVector2D InRange(270.0f, 360.0f);
		FVector2D OutRange(-90.0f, 0.0f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABaseCharacter::TurnInPlace(float DeltaTime)
{
	// AO_Yaw ���� 0���� ���ʹ��� -, �����ʹ��� +
	if (AO_Yaw > 90.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)//ȸ�����̶��
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.0f, DeltaTime, 4.0f);
		AO_Yaw = InterpAO_Yaw;

		if (FMath::Abs(AO_Yaw) < 15.0f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		}
	}
}

void ABaseCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABaseCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool ABaseCharacter::IsWeaponEquipped()
{
	//CombatComponent�� �ְ� ������ ���Ⱑ �ִٸ� true ����
	return (Combat && Combat->EquippedWeapon);
}

bool ABaseCharacter::IsAiming()
{	//Combat�� �ְ� bAiming ������ true��� ���� ���̴�.
	return (Combat && Combat->bAiming);
}

AWeapon* ABaseCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) // ���Ⱑ ���ٸ�
		return nullptr;

	return Combat->EquippedWeapon; // ������ ���� return
}
