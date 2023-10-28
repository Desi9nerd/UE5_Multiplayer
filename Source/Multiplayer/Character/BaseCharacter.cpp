#include "BaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Multiplayer/Weapon/Weapon.h"
#include "Multiplayer/Components/CombatComponent.h"
#include "Multiplayer/Components/BuffComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BaseCharacterAnimInstance.h"
#include "Multiplayer/Multiplayer.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Multiplayer/GameMode/MultiplayerGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Multiplayer/PlayerState/MultiplayerPlayerState.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;// ���� �� ��ġ�� ���������� ��ġ���ʰ� ���� �̵��Ͽ� ���� ��Ų��.

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

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true); // Replicated ���ش�.

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); //ĸ���� ī�޶� ������ �浹�� ����.
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); //�Ž��� ī�޶� ������ �浹�� ����.
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 0.0f, 850.0f);//FPS ���ӿ��� ȭ�� ȸ���� ������ �ǵ��� ���� 850���� �÷��ش�.

	TurningInPlace = ETurningInPlace::ETIP_NotTurning; //TurningInPlace �⺻�� ����.
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Client�鵵 �Ʒ��� �������� �� �� �ֵ��� Replicated ���ش�. Server�� �����ϸ� Client�鵵 �� �� �ְ� �ȴ�.
	DOREPLIFETIME_CONDITION(ABaseCharacter, OverlappingWeapon, COND_OwnerOnly);//OwnerOnly: �ش� ĳ���͸� ������ �ִ� Client�� ����
	DOREPLIFETIME(ABaseCharacter, Health);
	DOREPLIFETIME(ABaseCharacter, bDisableGameplay);
}

void ABaseCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();// Actor.h�� �ִ� �����Լ�

	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.0f; // ������ �������� Replicated�� �� ����� �ð��� 0���� �ʱ�ȭ
}

void ABaseCharacter::Elim() // Server Only
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}

	MulticastElim();

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABaseCharacter::ElimTimerFinished,
		ElimDelay
	); // SetTimer �� ElimTimerFinished() �Լ� ȣ��
}

void ABaseCharacter::MulticastElim_Implementation() // RPC
{
	if (MainPlayerController)
	{
		MainPlayerController->SetHUDWeaponAmmo(0); // ĳ���Ͱ� ������(=Elim) �Ѿ� �� 0���� ������Ʈ
	}

	bElimmed = true;
	PlayElimMontage();

	//** Dissolve ȿ�� ����
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	//** ĳ���� ������ ����
	bDisableGameplay = true; // true�� ĳ���� ������ ����. ���콺 ȸ������ �þ� ȸ���� ����
	GetCharacterMovement()->DisableMovement(); // ĳ���� ������ ����
	if (IsValid(Combat))
	{
		Combat->FireButtonPressed(false); // �߻� ��ư ����
	}

	//** �浹X. Disable Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Capsule �浹 ����
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Mesh �浹 ����

	// Elim Bot ������Ű��
	if (IsValid(ElimBotEffect))
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);//ĳ���� ��ġ(Z�� +200)
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		); // Elim Bot Effect ���� ��Ű��. ElimBotComponent ������ ���.
	}
	if (IsValid(ElimBotSound)) 
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		); // Elim Bot Sound 
	}

	//** ������ Scope 
	bool bHideSniperScope = IsLocallyControlled() &&
		Combat &&
		Combat->bAiming &&
		Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}
}

void ABaseCharacter::ElimTimerFinished()
{
	TWeakObjectPtr<AMultiplayerGameMode> MultiplayerGameMode = GetWorld()->GetAuthGameMode<AMultiplayerGameMode>();
	if (MultiplayerGameMode.IsValid())
	{
		MultiplayerGameMode->RequestRespawn(this, Controller); // ������
	}
}

void ABaseCharacter::Destroyed()
{
	Super::Destroyed();

	if (IsValid(ElimBotComponent))
	{
		ElimBotComponent->DestroyComponent(); // Elim Bot �Ҹ�
	}

	TWeakObjectPtr<AMultiplayerGameMode> MultiplayerGameMode = Cast<AMultiplayerGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = MultiplayerGameMode.IsValid() && MultiplayerGameMode->GetMatchState() != MatchState::InProgress; // MultiplayerGameMode�� �ְ� MatchState�� ��� ���� �ƴ϶�� true
	if (IsValid(Combat) && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy(); // �÷��̾ ������ ���� �Ҹ�
	}
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABaseCharacter::ReceiveDamage); // Delegate ���
	}
	if (IsValid(AttachedGrenade))
	{
		AttachedGrenade->SetVisibility(false); // ���� ���� �� ����ź �� ���̰� ����.
	}
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit(); // HUD�� ����/�¸�Ƚ�� �ű�� ���� Ŭ���� �ʱ�ȭ
}

void ABaseCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;// ������ �������� Replicated�� �� ��� �ð��� ��� ���
		if (TimeSinceLastMovementReplication > 0.25f) // ���� 0.25�� ������ �������� Replicated ���ش�
		{
			OnRep_ReplicatedMovement();
		}

		CalculateAO_Pitch();
	}
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABaseCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABaseCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABaseCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABaseCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABaseCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABaseCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABaseCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ABaseCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ABaseCharacter::GrenadeButtonPressed);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABaseCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABaseCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABaseCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABaseCharacter::LookUp);
}

void ABaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (IsValid(Combat))
	{
		Combat->Character = this;
	}
	if (IsValid(Buff))
	{
		Buff->Character = this;
	}
}

void ABaseCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABaseCharacter::PlayReloadMontage() // ������ ��Ÿ�� ���
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	
	TWeakObjectPtr<UAnimInstance> AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance.IsValid() && IsValid(ReloadMontage))
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABaseCharacter::PlayElimMontage()
{
	TWeakObjectPtr<UAnimInstance> AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance.IsValid() && IsValid(ElimMontage))
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABaseCharacter::PlayThrowGrenadeMontage()
{
	TWeakObjectPtr<UAnimInstance> AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance.IsValid() && IsValid(ThrowGrenadeMontage))
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABaseCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return; //���� �� ��� �������� return

	//** �ǰ� ��Ÿ�� ���
	TWeakObjectPtr<UAnimInstance> AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance.IsValid() && IsValid(HitReactMontage))
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABaseCharacter::GrenadeButtonPressed()
{
	if (IsValid(Combat))
	{
		Combat->ThrowGrenade();
	}
}

void ABaseCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed) return; // ���� ó��. ���� ���¸� ������X 

	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth); // ü�� - ������

	UpdateHUDHealth();
	PlayHitReactMontage();

	if (Health == 0.0f)
	{
		TWeakObjectPtr<AMultiplayerGameMode> MultiplayerGameMode = GetWorld()->GetAuthGameMode<AMultiplayerGameMode>();
		if (MultiplayerGameMode.IsValid())
		{
			MainPlayerController = MainPlayerController == nullptr ? Cast<AMainPlayerController>(Controller) : MainPlayerController;

			TObjectPtr<AMainPlayerController> AttackerController = Cast<AMainPlayerController>(InstigatorController);
			MultiplayerGameMode->PlayerEliminated(this, MainPlayerController, AttackerController);
		}
	}
}

void ABaseCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return;

	if (Controller != nullptr && Value != 0.0f)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);//ĳ������ Yaw���� �����Ϳ��� -90���� ���ȱ� ������ Controller->GetControlRotation().Yaw�� �ٲ� �� ���.
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));//������ Yaw�� X������ FVector
		AddMovementInput(Direction, Value);//�� �������� Value��ŭ �̵�
	}
}

void ABaseCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;

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
	if (bDisableGameplay) return;

	if (IsValid(Combat))
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
	if (IsValid(Combat))
	{
		Combat->EquipWeapon(OverlappingWeapon);//���⸦ �ݰ� ĳ���Ϳ� ������Ų��
	}
}

void ABaseCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch(); // �𸮾� �����Լ� Crouch()���
	}
}

void ABaseCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;

	if (IsValid(Combat))
	{
		Combat->Reload();
	}
}

void ABaseCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;

	if (IsValid(Combat))
	{
		Combat->SetAiming(true);
	}
}

void ABaseCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;

	if (IsValid(Combat))
	{
		Combat->SetAiming(false);
	}
}

float ABaseCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;

	return Velocity.Size();
}

void ABaseCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.0f && bIsInAir == false) // standing O, ���� X
	{
		bRotateRootBone = true; // RootBone ���� ȸ��O
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
		bRotateRootBone = false; // RootBone ���� ȸ��X
		StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		AO_Yaw = 0.0f;
		bUseControllerRotationYaw = true; // ���콺 ȸ�� �� ĳ���Ͱ� ���� ������ true ����
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABaseCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;

	if (AO_Pitch > 90.0f && IsLocallyControlled() == false)// 90���� �Ѱ�, ȣ��Ʈ���
	{
		// AO_Pitch�� ������ �����Ѵ�.from [270, 360) to [-90, 0)
		FVector2D InRange(270.0f, 360.0f);
		FVector2D OutRange(-90.0f, 0.0f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABaseCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if (Speed > 0.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;
	
	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}

		return;
	}

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABaseCharacter::Jump()
{
	if (bDisableGameplay) return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABaseCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(true); 
	}
}

void ABaseCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(false);
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

void ABaseCharacter::HideCameraIfCharacterClose()
{
	if (IsLocallyControlled() == false) return;

	// 'ī�޶� ��ġ - ĳ���� ��ġ < CameraThreshold�� ���� ��'�̸�
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false); // ĳ������ �Ž��� ���ش�
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh()) 
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;// ������ �Ž��� ���ش�
		}
	}
	else 
	{
		GetMesh()->SetVisibility(true); // ĳ������ �Ž��� ���ش�.
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;// ������ �Ž��� ���ش�
		}
	}
}

void ABaseCharacter::OnRep_Health(float LastHealth) // ĳ���� ü�� ��ȭ
{
	UpdateHUDHealth();
	if (Health < LastHealth) // ü���� ���̴� ��Ȳ�̸�
	{
		PlayHitReactMontage(); // �ǰ� ��Ÿ�� ���
	}
}

void ABaseCharacter::UpdateHUDHealth()
{
	MainPlayerController = MainPlayerController == nullptr ? Cast<AMainPlayerController>(Controller) : MainPlayerController;
	if (IsValid(MainPlayerController))
	{
		MainPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABaseCharacter::PollInit()
{
	if (MultiplayerPlayerState == nullptr)
	{
		MultiplayerPlayerState = GetPlayerState<AMultiplayerPlayerState>();
		if (MultiplayerPlayerState.IsValid())
		{
			MultiplayerPlayerState->AddToScore(0.0f); // ���� �ʱ�ȭ
			MultiplayerPlayerState->AddToDefeats(0); // �¸�Ƚ�� �ʱ�ȭ
		}
	}
}

void ABaseCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (IsValid(DynamicDissolveMaterialInstance))
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABaseCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABaseCharacter::UpdateDissolveMaterial); // Delegate
	if (IsValid(DissolveCurve) && IsValid(DissolveTimeline))
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
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

FVector ABaseCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector(); //nullptr�� �� FVector ����

	return Combat->HitTarget; // CombatComponent�� HitTaget�� return
}

ECombatState ABaseCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;

	return Combat->CombatState;
}
