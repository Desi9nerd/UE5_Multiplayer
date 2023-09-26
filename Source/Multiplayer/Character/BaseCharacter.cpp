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
	CameraSpringArm->SetupAttachment(GetMesh());//Mesh 아래 항목으로 붙인다.
	CameraSpringArm->TargetArmLength = 600.0f;
	CameraSpringArm->bUsePawnControlRotation = true;//true: 마우스를 움직일 때 controller를 따라 SpringArm를 회전시킬 수 있다

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false; //Unequipped로 시작했을때 마우스를 돌릴때 화면만 돌아가지 않고 플레이어와 같이 돌아가도록 false 설정.
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true); //Combat을 Replicated 컴포넌트로 만들어준다.

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); //캡슐과 카메라 사이의 충돌을 꺼줌.
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); //매쉬과 카메라 사이의 충돌을 꺼줌.

	TurningInPlace = ETurningInPlace::ETIP_NotTurning; //TurningInPlace 기본값 설정.
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

	DOREPLIFETIME_CONDITION(ABaseCharacter, OverlappingWeapon, COND_OwnerOnly);//OwnerOnly: 해당 캐릭터를 가지고 있는 Client만 적용
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
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);//캐릭터의 Yaw값을 에디터에서 -90도로 돌렸기 때문에 Controller->GetControlRotation().Yaw로 바뀐 값 사용.
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));//수정된 Yaw의 X방향의 FVector
		AddMovementInput(Direction, Value);//앞 방향으로 Value만큼 이동
	}
}

void ABaseCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.0f)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));//수정된 Yaw의 Y방향의 FVector
		AddMovementInput(Direction, Value);//오른쪽 방향으로 Value만큼 이동
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
		if (HasAuthority()) //서버 기준. Server에서 validate하는 HasAuthority()
		{
			Combat->EquipWeapon(OverlappingWeapon); //무기를 주어서 캐릭터에 장착시킨다
		}
		else //클라이언트 기준. Authority가 없는 경우, RPC를 통해 ServerEquipButtonPressed_Implementation()에서 무기장착 수행
		{
			ServerEquipButtonPressed(); //무기 장착
		}
	}
}

void ABaseCharacter::ServerEquipButtonPressed_Implementation()
{
	// 서버를 통해 무기 장착하는 경우
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);//무기를 줍고 캐릭터에 장착시킨다
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
		Crouch(); // 언리얼 내장함수 Crouch()사용
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

	if (Speed == 0.0f && bIsInAir == false) // standing O, 점프 X
	{
		FRotator CurrentAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation); // 회전값(=여기서는 Yaw값) Normalize
		AO_Yaw = DeltaAimRotation.Yaw; // Yaw값 설정.
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;// 회전하지않는 정지상태라면 InterpAO_Yaw=AO_Yaw 설정
		}
		bUseControllerRotationYaw = true; //정지 시 마우스 회전으로 카메라 회전이 가능하지 않도록 true 설정.
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.0f || bIsInAir) // running O 또는 점프 O
	{
		StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		AO_Yaw = 0.0f;
		bUseControllerRotationYaw = true; // 마우스 회전 시 캐릭터가 같이 돌도록 true 설정
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.0f && IsLocallyControlled() == false)// 90도가 넘고, 호스트라면
	{
		// AO_Pitch값 범위를 제한한다.from [270, 360) to [-90, 0)
		FVector2D InRange(270.0f, 360.0f);
		FVector2D OutRange(-90.0f, 0.0f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABaseCharacter::TurnInPlace(float DeltaTime)
{
	// AO_Yaw 정면 0에서 왼쪽방향 -, 오른쪽방향 +
	if (AO_Yaw > 90.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)//회전중이라면
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
	//CombatComponent가 있고 장착된 무기가 있다면 true 리턴
	return (Combat && Combat->EquippedWeapon);
}

bool ABaseCharacter::IsAiming()
{	//Combat이 있고 bAiming 변수가 true라면 조준 중이다.
	return (Combat && Combat->bAiming);
}

AWeapon* ABaseCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) // 무기가 없다면
		return nullptr;

	return Combat->EquippedWeapon; // 장착된 무기 return
}
