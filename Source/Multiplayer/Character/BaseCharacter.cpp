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

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;// 스폰 시 위치가 겹쳐있으면 겹치지않게 조금 이동하여 스폰 시킨다.

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

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true); // Replicated 해준다.

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); //캡슐과 카메라 사이의 충돌을 꺼줌.
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); //매쉬과 카메라 사이의 충돌을 꺼줌.
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 0.0f, 850.0f);//FPS 게임에서 화면 회전이 빠르게 되도록 값을 850으로 올려준다.

	TurningInPlace = ETurningInPlace::ETIP_NotTurning; //TurningInPlace 기본값 설정.
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

	// Client들도 아래의 변수들을 알 수 있도록 Replicated 해준다. Server에 세팅하면 Client들도 알 수 있게 된다.
	DOREPLIFETIME_CONDITION(ABaseCharacter, OverlappingWeapon, COND_OwnerOnly);//OwnerOnly: 해당 캐릭터를 가지고 있는 Client만 적용
	DOREPLIFETIME(ABaseCharacter, Health);
	DOREPLIFETIME(ABaseCharacter, bDisableGameplay);
}

void ABaseCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();// Actor.h에 있는 내장함수

	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.0f; // 마지막 움직임이 Replicated된 후 경과한 시간을 0으로 초기화
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
	); // SetTimer 후 ElimTimerFinished() 함수 호출
}

void ABaseCharacter::MulticastElim_Implementation() // RPC
{
	if (MainPlayerController)
	{
		MainPlayerController->SetHUDWeaponAmmo(0); // 캐릭터가 죽으면(=Elim) 총알 수 0으로 업데이트
	}

	bElimmed = true;
	PlayElimMontage();

	//** Dissolve 효과 시작
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	//** 캐릭터 움직임 제한
	bDisableGameplay = true; // true면 캐릭터 움직임 제한. 마우스 회전으로 시야 회전은 가능
	GetCharacterMovement()->DisableMovement(); // 캐릭터 움직임 제한
	if (IsValid(Combat))
	{
		Combat->FireButtonPressed(false); // 발사 버튼 방지
	}

	//** 충돌X. Disable Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Capsule 충돌 꺼줌
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Mesh 충돌 꺼줌

	// Elim Bot 스폰시키기
	if (IsValid(ElimBotEffect))
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);//캐릭터 위치(Z만 +200)
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		); // Elim Bot Effect 스폰 시키기. ElimBotComponent 변수에 담기.
	}
	if (IsValid(ElimBotSound)) 
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		); // Elim Bot Sound 
	}

	//** 저격총 Scope 
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
		MultiplayerGameMode->RequestRespawn(this, Controller); // 리스폰
	}
}

void ABaseCharacter::Destroyed()
{
	Super::Destroyed();

	if (IsValid(ElimBotComponent))
	{
		ElimBotComponent->DestroyComponent(); // Elim Bot 소멸
	}

	TWeakObjectPtr<AMultiplayerGameMode> MultiplayerGameMode = Cast<AMultiplayerGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = MultiplayerGameMode.IsValid() && MultiplayerGameMode->GetMatchState() != MatchState::InProgress; // MultiplayerGameMode가 있고 MatchState이 경기 중이 아니라면 true
	if (IsValid(Combat) && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy(); // 플레이어에 장착된 무기 소멸
	}
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABaseCharacter::ReceiveDamage); // Delegate 등록
	}
	if (IsValid(AttachedGrenade))
	{
		AttachedGrenade->SetVisibility(false); // 게임 시작 시 수류탄 안 보이게 설정.
	}
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit(); // HUD와 점수/승리횟수 매기기 관련 클래스 초기화
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
		TimeSinceLastMovementReplication += DeltaTime;// 마지막 움직임이 Replicated된 후 경과 시간을 계속 기록
		if (TimeSinceLastMovementReplication > 0.25f) // 값이 0.25가 넘으면 움직임을 Replicated 해준다
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

void ABaseCharacter::PlayReloadMontage() // 재장전 몽타주 재생
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
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return; //무기 안 들고 있을때는 return

	//** 피격 몽타주 재생
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
	if (bElimmed) return; // 예외 처리. 죽은 상태면 데미지X 

	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth); // 체력 - 데미지

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
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);//캐릭터의 Yaw값을 에디터에서 -90도로 돌렸기 때문에 Controller->GetControlRotation().Yaw로 바뀐 값 사용.
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));//수정된 Yaw의 X방향의 FVector
		AddMovementInput(Direction, Value);//앞 방향으로 Value만큼 이동
	}
}

void ABaseCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;

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
	if (bDisableGameplay) return;

	if (IsValid(Combat))
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
	if (IsValid(Combat))
	{
		Combat->EquipWeapon(OverlappingWeapon);//무기를 줍고 캐릭터에 장착시킨다
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
		Crouch(); // 언리얼 내장함수 Crouch()사용
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

	if (Speed == 0.0f && bIsInAir == false) // standing O, 점프 X
	{
		bRotateRootBone = true; // RootBone 기준 회전O
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
		bRotateRootBone = false; // RootBone 기준 회전X
		StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		AO_Yaw = 0.0f;
		bUseControllerRotationYaw = true; // 마우스 회전 시 캐릭터가 같이 돌도록 true 설정
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABaseCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;

	if (AO_Pitch > 90.0f && IsLocallyControlled() == false)// 90도가 넘고, 호스트라면
	{
		// AO_Pitch값 범위를 제한한다.from [270, 360) to [-90, 0)
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

void ABaseCharacter::HideCameraIfCharacterClose()
{
	if (IsLocallyControlled() == false) return;

	// '카메라 위치 - 캐릭터 위치 < CameraThreshold로 정한 값'이면
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false); // 캐릭터의 매쉬를 꺼준다
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh()) 
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;// 무기의 매쉬를 꺼준다
		}
	}
	else 
	{
		GetMesh()->SetVisibility(true); // 캐릭터의 매쉬를 켜준다.
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;// 무기의 매쉬를 켜준다
		}
	}
}

void ABaseCharacter::OnRep_Health(float LastHealth) // 캐릭터 체력 변화
{
	UpdateHUDHealth();
	if (Health < LastHealth) // 체력이 깎이는 상황이면
	{
		PlayHitReactMontage(); // 피격 몽타주 재생
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
			MultiplayerPlayerState->AddToScore(0.0f); // 점수 초기화
			MultiplayerPlayerState->AddToDefeats(0); // 승리횟수 초기화
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

FVector ABaseCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector(); //nullptr면 빈 FVector 리턴

	return Combat->HitTarget; // CombatComponent의 HitTaget을 return
}

ECombatState ABaseCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;

	return Combat->CombatState;
}
