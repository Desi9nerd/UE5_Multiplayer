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
#include "Components/BoxComponent.h"
#include "Multiplayer/Components/LagCompensationComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;// ���� �� ��ġ�� ���������� ��ġ���ʰ� ���� �̵��Ͽ� ���� ��Ų��.

	BackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackpactMesh"));
	BackpackMesh->SetupAttachment(GetMesh(), FName("BackpackSocket"));
	BackpackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

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

	//** Hit Boxes used for Server-side Rewind	
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("head"), head);
	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	pelvis->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);
	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	spine_02->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);
	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	spine_03->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);
	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	upperarm_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);
	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	upperarm_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);
	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	lowerarm_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);
	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	lowerarm_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);
	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	hand_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);
	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	hand_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);
	blanket = CreateDefaultSubobject<UBoxComponent>(TEXT("blanket"));
	blanket->SetupAttachment(GetMesh(), FName("backpack"));
	blanket->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("blanket"), blanket);
	backpack = CreateDefaultSubobject<UBoxComponent>(TEXT("backpack"));
	backpack->SetupAttachment(GetMesh(), FName("backpack"));
	backpack->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("backpack"), backpack);
	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	thigh_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);
	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	thigh_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);
	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	calf_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);
	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	calf_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);
	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	foot_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);
	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	foot_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Client�鵵 �Ʒ��� �������� �� �� �ֵ��� Replicated ���ش�. Server�� �����ϸ� Client�鵵 �� �� �ְ� �ȴ�.
	DOREPLIFETIME_CONDITION(ABaseCharacter, OverlappingWeapon, COND_OwnerOnly);//OwnerOnly: �ش� ĳ���͸� ������ �ִ� Client�� ����
	DOREPLIFETIME(ABaseCharacter, Health);
	DOREPLIFETIME(ABaseCharacter, Shield);
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
	DropOrDestroyWeapons(); // ������ ����� Secondary ���⸦ ����߸��ų� �Ҹ��Ŵ

	MulticastElim();

	GetWorldTimerManager().SetTimer(ElimTimer, this, &ABaseCharacter::ElimTimerFinished, ElimDelay); // SetTimer �� ElimTimerFinished() �Լ� ȣ��
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

void ABaseCharacter::DropOrDestroyWeapon(AWeapon* Weapon) // ���⸦ ����߸��ų� �Ҹ��Ŵ
{
	if (Weapon == nullptr) return;

	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy(); // ���� �Ҹ�
	}
	else
	{
		Weapon->Dropped(); // ���� ����߸���
	}
}

void ABaseCharacter::DropOrDestroyWeapons() // ������ ����� Secondary ���⸦ ����߸��ų� �Ҹ��Ŵ
{
	if (IsValid(Combat))
	{
		if (Combat->EquippedWeapon) // ������ ���Ⱑ ������
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon); // ������ ���⸦ ����߸��ų� �Ҹ��Ŵ
		}
		if (Combat->SecondaryWeapon) // Secondary ���Ⱑ ������
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon); // Secondary ���⸦ ����߸��ų� �Ҹ��Ŵ
		}
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

	SpawnDefaultWeapon();
	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();

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
		Buff->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched); // Speed �ʱⰪ ����
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity); // Jump �ʱⰪ ����
	}
	if (IsValid(LagCompensation))
	{
		LagCompensation->Character = this;
		if (IsValid(Controller))
		{
			LagCompensation->Controller = Cast<AMainPlayerController>(Controller);
		}
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

	float DamageToHealth = Damage;
	if (Shield > 0.0f) // �ǵ尡 0�̻��̸� ü���� ��� ���� �ǵ带 ���� ���. 
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.0f, MaxShield);
			DamageToHealth = 0.0f;
		}
		else // �������� ���� �ǵ��ġ ���� ���� ���
		{
			Shield = 0.0f;
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.0f, Damage); // �ǵ带 ��� ���� ������
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.0f, MaxHealth); // ü�� - ������

	UpdateHUDHealth();
	UpdateHUDShield();
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
		//if (HasAuthority()) //���� ����. Server���� validate�ϴ� HasAuthority()
		//{
		//	Combat->EquipWeapon(OverlappingWeapon); //���⸦ �־ ĳ���Ϳ� ������Ų��
		//}
		//else //Ŭ���̾�Ʈ ����. Authority�� ���� ���, RPC�� ���� /ServerEquipButtonPressed_Implementation/()���� �������� ����
		//{
		//	ServerEquipButtonPressed(); //���� ����
		//}
		// ���� �ڵ带 �Ʒ� �ڵ�� ����. ���� �ڵ� ���� ����.

		// Authority�� �� ���� ��
		// Authority�� ���� ���, ServerEquipButtonPressed()�� �������� ����ȴ�.
		// Authority�� �ִ� ��쵵 ServerEquipButtonPressed()�� �������� ����ȴ�.
		ServerEquipButtonPressed(); //���� ����
	}
}

void ABaseCharacter::ServerEquipButtonPressed_Implementation() // E ��ư�� ������ �� �Լ�
{
	// ������ ���� ���� �����ϴ� ���
	if (IsValid(Combat))
	{
		if (IsValid(OverlappingWeapon)) // (AreaSphere�� ��ġ��)���Ⱑ �ִٸ�
		{
			Combat->EquipWeapon(OverlappingWeapon); //���⸦ �ݰ� ĳ���Ϳ� ������Ų��
		}
		// ���Ⱑ ��ġ�� ���°� �ƴѵ� Primary�� Secondary ���� ��� �ִ� ���
		else if (Combat->ShouldSwapWeapons()) 
		{
			Combat->SwapWeapons(); // Primary ����� Secondary ���⸦ ��ü�Ѵ�
		}
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

void ABaseCharacter::OnRep_Shield(float LastShield) // ĳ���� �ǵ� ��ȭ
{
	UpdateHUDShield();
	if (Shield < LastShield) // �ǵ尡 ���̴� ��Ȳ�̸�
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

void ABaseCharacter::UpdateHUDShield()
{
	MainPlayerController = MainPlayerController == nullptr ? Cast<AMainPlayerController>(Controller) : MainPlayerController;
	if (IsValid(MainPlayerController))
	{
		MainPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABaseCharacter::UpdateHUDAmmo()
{
	MainPlayerController = MainPlayerController == nullptr ? Cast<AMainPlayerController>(Controller) : MainPlayerController;
	if (IsValid(MainPlayerController) && IsValid(Combat) && Combat->EquippedWeapon)
	{
		MainPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo); // ���� źâ HUD ������Ʈ
		MainPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo()); // �������⿡ ������ �Ѿ� HUD �����̤�
	}
}

void ABaseCharacter::SpawnDefaultWeapon() // ���ӽ��� �� �⺻���� ����
{
	TWeakObjectPtr<AMultiplayerGameMode> MultiplayerGameMode = Cast<AMultiplayerGameMode>(UGameplayStatics::GetGameMode(this));
	TWeakObjectPtr<UWorld> World = GetWorld();

	if (MultiplayerGameMode.IsValid() && World.IsValid() && bElimmed == false && IsValid(DefaultWeaponClass))
	{
		TWeakObjectPtr<AWeapon> StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true; // �⺻���� ���� true. ĳ���Ͱ� ������(=Elim) �ش� ���� �Ҹ�.
		if (IsValid(Combat))
		{
			Combat->EquipWeapon(StartingWeapon.Get());
		}
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

bool ABaseCharacter::IsLocallyReloading()
{
	if (Combat == nullptr) return false;

	return Combat->bLocallyReloading;
}
