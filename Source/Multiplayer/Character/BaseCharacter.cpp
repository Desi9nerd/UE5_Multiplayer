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
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Multiplayer/GameState/MultiplayerGameState.h"
#include "Multiplayer/PlayerStart/TeamPlayerStart.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;// 스폰 시 위치가 겹쳐있으면 겹치지않게 조금 이동하여 스폰 시킨다.

	BackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackpactMesh"));
	BackpackMesh->SetupAttachment(GetMesh(), FName("BackpackSocket"));
	BackpackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); //캡슐과 카메라 사이의 충돌을 꺼줌.
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore); //매쉬과 카메라 사이의 충돌을 꺼줌.
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 0.0f, 850.0f);//FPS 게임에서 화면 회전이 빠르게 되도록 값을 850으로 올려준다.

	TurningInPlace = ETurningInPlace::ETIP_NotTurning; //TurningInPlace 기본값 설정.
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//** Hit Boxes used for Server-side Rewind	
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("head"), head);
	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);
	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);
	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);
	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);
	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);
	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);
	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);
	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);
	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);
	blanket = CreateDefaultSubobject<UBoxComponent>(TEXT("blanket"));
	blanket->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("blanket"), blanket);
	backpack = CreateDefaultSubobject<UBoxComponent>(TEXT("backpack"));
	backpack->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("backpack"), backpack);
	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);
	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);
	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);
	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);
	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);
	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox); // Multiplayer.h에 선언된 ECC_HitBox
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Client들도 아래의 변수들을 알 수 있도록 Replicated 해준다. Server에 세팅하면 Client들도 알 수 있게 된다.
	DOREPLIFETIME_CONDITION(ABaseCharacter, OverlappingWeapon, COND_OwnerOnly);//OwnerOnly: 해당 캐릭터를 가지고 있는 Client만 적용
	DOREPLIFETIME(ABaseCharacter, Health);
	DOREPLIFETIME(ABaseCharacter, Shield);
	DOREPLIFETIME(ABaseCharacter, bDisableGameplay);
}

void ABaseCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();// Actor.h에 있는 내장함수

	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.0f; // 마지막 움직임이 Replicated된 후 경과한 시간을 0으로 초기화
}

void ABaseCharacter::Elim(bool bPlayerLeftGame) // Server Only
{
	DropOrDestroyWeapons(); // 장착된 무기와 Secondary 무기를 떨어뜨리거나 소멸시킴

	MulticastElim(bPlayerLeftGame);
}

void ABaseCharacter::MulticastElim_Implementation(bool bPlayerLeftGame) // RPC
{
	bLeftGame = bPlayerLeftGame;

	if (MainPlayerController)
	{
		MainPlayerController->SetHUDWeaponAmmo(0); // 캐릭터가 죽으면(=Elim) 총알 수 0으로 업데이트
	}

	bElimmed = true;
	PlayElimMontage();

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
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 수류탄 Mesh 충돌 꺼줌
	
	// Elim Coin 스폰시키기
	if (IsValid(ElimCoinEffect))
	{
		FVector ElimCoinSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z - 50.f);//캐릭터 위치(Z만 -50)
		ElimCoinComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ElimCoinEffect,
			ElimCoinSpawnPoint,
			GetActorRotation()
		); // Elim Coin Effect 스폰 시키기. ElimCoinComponent 변수에 담기.
	}
	if (IsValid(ElimCoinSound))
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimCoinSound,
			GetActorLocation()
		); // Elim Coin Sound 
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

	//** Crown을 가지고 있는 플레이어가 죽을때 Crown 없애기
	if (IsValid(CrownComponent))
	{
		CrownComponent->DestroyComponent();
	}

	//** SetTimer 후 ElimTimerFinished() 함수 호출
	GetWorldTimerManager().SetTimer(ElimTimer, this, &ABaseCharacter::ElimTimerFinished, ElimDelay);
}

void ABaseCharacter::ElimTimerFinished()
{
	MGameMode = MGameMode == nullptr ? GetWorld()->GetAuthGameMode<AMultiplayerGameMode>() : MGameMode;
	if (IsValid(MGameMode) && false == bLeftGame) // 게임퇴장X
	{
		MGameMode->RequestRespawn(this, Controller); // 리스폰
	}
	if (IsLocallyControlled() && bLeftGame) // 게임퇴장O
	{
		// 해당 플레이어만 게임에서 퇴장한다. 다른 Client들에서는 Broadcast되지 않도록 IsLocallyControlled()일 때만 Broadcast한다.
		OnLeftGame.Broadcast();
	}
}

void ABaseCharacter::ServerLeaveGame_Implementation() // 게임 퇴장
{
	MGameMode = MGameMode == nullptr ? GetWorld()->GetAuthGameMode<AMultiplayerGameMode>() : MGameMode;
	MultiplayerPlayerState = MultiplayerPlayerState == nullptr ? GetPlayerState<AMultiplayerPlayerState>() : MultiplayerPlayerState;
	if (IsValid(MGameMode) && IsValid(MultiplayerPlayerState))
	{
		MGameMode->PlayerLeftGame(MultiplayerPlayerState.Get());
	}
}

void ABaseCharacter::DropOrDestroyWeapon(AWeapon* Weapon) // 무기를 떨어뜨리거나 소멸시킴
{
	if (false == IsValid(Weapon)) return;

	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy(); // 무기 소멸
	}
	else
	{
		Weapon->Dropped(); // 무기 떨어뜨리기
	}
}

void ABaseCharacter::DropOrDestroyWeapons() // 장착된 무기와 Secondary 무기를 떨어뜨리거나 소멸시킴
{
	if (IsValid(Combat))
	{
		if (Combat->EquippedWeapon) // 장착된 무기가 있으면
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon); // 장착된 무기를 떨어뜨리거나 소멸시킴
		}
		if (Combat->SecondaryWeapon) // Secondary 무기가 있으면
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon); // Secondary 무기를 떨어뜨리거나 소멸시킴
		}
		if (Combat->TheFlag)
		{
			Combat->TheFlag->Dropped();
		}
	}
}

void ABaseCharacter::SetSpawnPoint() // 게임시작 위치 초기화
{
	// 팀이 설정된 경우, 위치를 override한다.
	if (HasAuthority() && MultiplayerPlayerState->GetTeam() != ETeam::ET_NoTeam)
	{
		TArray<TObjectPtr<AActor>> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), PlayerStarts); // PlayerStarts 배열에 ATeamPlayerStart::StaticClass()를 찾아 모두 담는다(=팀 배정이 된 캐릭터들을 모두 담는다)

		//** 팀 배정이 된 캐릭터들 중 같은 팀의 ATeamPlayerStart들을 TeamPlayerStarts 배열에 다 담는다. 
		TArray<TObjectPtr<ATeamPlayerStart>> TeamPlayerStarts;
		for (TObjectPtr<AActor> Start : PlayerStarts)
		{
			TObjectPtr<ATeamPlayerStart> TeamStart = Cast<ATeamPlayerStart>(Start);
			if (IsValid(TeamStart) && TeamStart->Team == MultiplayerPlayerState->GetTeam())
			{
				TeamPlayerStarts.Add(TeamStart);
			}
		}
		if (TeamPlayerStarts.Num() > 0)
		{
			TWeakObjectPtr<ATeamPlayerStart> ChosenPlayerStart = TeamPlayerStarts[FMath::RandRange(0, TeamPlayerStarts.Num() - 1)];
			SetActorLocationAndRotation(
				ChosenPlayerStart->GetActorLocation(),
				ChosenPlayerStart->GetActorRotation()
			); // 위치, 회전값 설정
		}
	}
}

void ABaseCharacter::OnPlayerStateInitialized() // 게임 시작 시 초기화
{
	MultiplayerPlayerState->AddToScore(0.0f); // 점수 초기화
	MultiplayerPlayerState->AddToDefeats(0); // 승리횟수 초기화
	SetTeamColor(MultiplayerPlayerState->GetTeam()); // Team에 따라 Team Color 초기화
	SetSpawnPoint(); // 게임 시작위치 초기화
} 

void ABaseCharacter::Destroyed()
{
	Super::Destroyed();
	
	if (IsValid(ElimCoinComponent))
	{
		ElimCoinComponent->DestroyComponent(); // Elim Coin 소멸
	}

	MGameMode = MGameMode == nullptr ? GetWorld()->GetAuthGameMode<AMultiplayerGameMode>() : MGameMode;
	bool bMatchNotInProgress = MGameMode && MGameMode->GetMatchState() != MatchState::InProgress; // MGameMode가 있고 MatchState이 경기 중이 아니라면 true
	if (IsValid(Combat) && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy(); // 플레이어에 장착된 무기 소멸
	}
}

void ABaseCharacter::MulticastGainedTheLead_Implementation() // 1등 Crown 띄우기
{
	if (false == IsValid(CrownSystem)) return;

	if (false == IsValid(CrownComponent)) // Crown이 새로 생기는 경우
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetMesh(),
			FName(),
			GetActorLocation() + FVector(0.0f, 0.0f, 120.0f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}

	if (IsValid(CrownComponent)) // 이미 Crown이 있는 상태에서 지속되는 경우. 1등이 계속 1등인 경우
	{
		CrownComponent->Activate(); // Crown 활성화
	}
}

void ABaseCharacter::MulticastLostTheLead_Implementation() // 1등에서 밀려나면 Crown 띄운거 없애기
{
	if (IsValid(CrownComponent))
	{
		CrownComponent->DestroyComponent(); // Crown 비활성화
	}
}

void ABaseCharacter::SetTeamColor(ETeam Team)
{
	if (GetMesh() == nullptr || RedMaterial_0 == nullptr) return; // 예외 처리

	switch (Team)
	{
	case ETeam::ET_NoTeam:
		GetMesh()->SetMaterial(0, RedMaterial_0);
		GetMesh()->SetMaterial(1, RedMaterial_1);
		GetMesh()->SetMaterial(2, RedMaterial_2);
		GetMesh()->SetMaterial(7, RedMaterial_7);
		GetMesh()->SetMaterial(8, RedMaterial_8);
		GetMesh()->SetMaterial(9, RedMaterial_9);
		BackpackMesh->SetMaterial(0, TransparentMaterial);
		break;
	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial_0);
		GetMesh()->SetMaterial(1, RedMaterial_1);
		GetMesh()->SetMaterial(2, RedMaterial_2);
		GetMesh()->SetMaterial(7, RedMaterial_7);
		GetMesh()->SetMaterial(8, RedMaterial_8);
		GetMesh()->SetMaterial(9, RedMaterial_9);
		BackpackMesh->SetMaterial(0, BackpackMaterial);
		break;
	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial_0);
		GetMesh()->SetMaterial(1, BlueMaterial_1);
		GetMesh()->SetMaterial(2, BlueMaterial_2);
		GetMesh()->SetMaterial(7, BlueMaterial_7);
		GetMesh()->SetMaterial(8, BlueMaterial_8);
		GetMesh()->SetMaterial(9, BlueMaterial_9);
		BackpackMesh->SetMaterial(0, TransparentMaterial);
		break;
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
	if (IsValid(Combat) && Combat->bHoldingTheFlag) // 깃발을 들고 있다면
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (Combat && Combat->EquippedWeapon) GetCharacterMovement()->bOrientRotationToMovement = false;
	if (Combat && Combat->EquippedWeapon) bUseControllerRotationYaw = true;

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
		Buff->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched); // Speed 초기값 설정
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity); // Jump 초기값 설정
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
	if (false == IsValid(Combat) || false == IsValid(Combat->EquippedWeapon)) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (IsValid(AnimInstance) && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABaseCharacter::PlayReloadMontage() // 재장전 몽타주 재생
{
	if (false == IsValid(Combat) || false == IsValid(Combat->EquippedWeapon)) return;
	
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

void ABaseCharacter::PlaySwapMontage()
{
	TWeakObjectPtr<UAnimInstance> AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance.IsValid() && IsValid(SwapMontage))
	{
		AnimInstance->Montage_Play(SwapMontage);
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
		if (Combat->bHoldingTheFlag) return; // 깃발 들고있을때 예외 처리

		Combat->ThrowGrenade();
	}
}

void ABaseCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	MGameMode = MGameMode == nullptr ? GetWorld()->GetAuthGameMode<AMultiplayerGameMode>() : MGameMode;
	if (bElimmed || MGameMode == nullptr) return; // 예외 처리. 죽은 상태면 데미지X

	Damage = MGameMode->CalculateDamage(InstigatorController, Controller, Damage);

	float DamageToHealth = Damage;
	if (Shield > 0.0f) // 실드가 0이상이면 체력을 깍기 전에 실드를 먼저 깐다. 
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.0f, MaxShield);
			DamageToHealth = 0.0f;
		}
		else // 데미지가 남은 실드수치 보다 높은 경우
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.0f, Damage); // 실드를 깍고 남은 데미지
			Shield = 0.0f;
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.0f, MaxHealth); // 체력 - 데미지

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	if (Health == 0.0f)
	{
		if (IsValid(MGameMode))
		{
			MainPlayerController = MainPlayerController == nullptr ? Cast<AMainPlayerController>(Controller) : MainPlayerController;

			TObjectPtr<AMainPlayerController> AttackerController = Cast<AMainPlayerController>(InstigatorController);
			MGameMode->PlayerEliminated(this, MainPlayerController, AttackerController);
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

	if (IsValid(Controller) && Value != 0.0f)
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
		// Authority를 콜 했을 때
		// Authority가 없는 경우, ServerEquipButtonPressed()가 서버에서 실행된다.
		// Authority가 있는 경우도 ServerEquipButtonPressed()가 서버에서 실행된다.

		if (Combat->bHoldingTheFlag) return;

		if (Combat->CombatState == ECombatState::ECS_Unoccupied) 
		{
			ServerEquipButtonPressed(); //무기 장착
		}

		bool bSwap = Combat->ShouldSwapWeapons() &&
			HasAuthority() == false &&
			Combat->CombatState == ECombatState::ECS_Unoccupied &&
			OverlappingWeapon == nullptr;
		if (bSwap)
		{
			PlaySwapMontage(); // 무기교체 몽타주 재생
			Combat->CombatState = ECombatState::ECS_SwappingWeapons;
			bFinishedSwapping = false;
		}
	}
}

void ABaseCharacter::ServerEquipButtonPressed_Implementation() // E 버튼을 누렀을 때 함수
{
	// 서버를 통해 무기 장착하는 경우
	if (IsValid(Combat))
	{
		if (IsValid(OverlappingWeapon)) // (AreaSphere에 겹치는)무기가 있다면
		{
			Combat->EquipWeapon(OverlappingWeapon); //무기를 줍고 캐릭터에 장착시킨다
		}
		// 무기가 겹치는 상태가 아닌데 Primary와 Secondary 무기 모두 있는 경우
		else if (Combat->ShouldSwapWeapons()) 
		{
			Combat->SwapWeapons(); // Primary 무기와 Secondary 무기를 교체한다
		}
	}
}

void ABaseCharacter::CrouchButtonPressed()
{
	if (Combat && Combat->bHoldingTheFlag) return;
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
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (IsValid(Combat))
	{
		Combat->Reload();
	}
}

void ABaseCharacter::AimButtonPressed()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (IsValid(Combat))
	{
		Combat->SetAiming(true);
	}
}

void ABaseCharacter::AimButtonReleased()
{
	if (Combat && Combat->bHoldingTheFlag) return;
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
	if (Combat && Combat->bHoldingTheFlag) return;
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
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(true); 
	}
}

void ABaseCharacter::FireButtonReleased()
{
	if (Combat && Combat->bHoldingTheFlag) return;
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
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;// 무기의 매쉬를 꺼준다
		}
	}
	else 
	{
		GetMesh()->SetVisibility(true); // 캐릭터의 매쉬를 켜준다.
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;// 무기의 매쉬를 켜준다
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;// 무기의 매쉬를 켜준다
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

void ABaseCharacter::OnRep_Shield(float LastShield) // 캐릭터 실드 변화
{
	UpdateHUDShield();
	if (Shield < LastShield) // 실드가 깎이는 상황이면
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
		MainPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo); // 가진 탄창 HUD 업데이트
		MainPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo()); // 장착무기에 장전된 총알 HUD 업데이ㅡ
	}
}

void ABaseCharacter::SpawnDefaultWeapon() // 게임시작 시 기본무기 생성
{
	MGameMode = MGameMode == nullptr ? GetWorld()->GetAuthGameMode<AMultiplayerGameMode>() : MGameMode;

	TWeakObjectPtr<UWorld> World = GetWorld();

	if (IsValid(MGameMode) && World.IsValid() && bElimmed == false && IsValid(DefaultWeaponClass))
	{
		TWeakObjectPtr<AWeapon> StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true; // 기본무기 생성 true. 캐릭터가 죽으면(=Elim) 해당 무기 소멸.
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
		if (IsValid(MultiplayerPlayerState))
		{
			OnPlayerStateInitialized(); // 초기화

			TWeakObjectPtr<AMultiplayerGameState> MultiplayerGameState = Cast<AMultiplayerGameState>(UGameplayStatics::GetGameState(this));
			if (MultiplayerGameState.IsValid() && 
				MultiplayerGameState->TopScoringPlayers.Contains(MultiplayerPlayerState))
			{
				MulticastGainedTheLead(); // 1등 Crown 띄우기
			}
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
	if (false == IsValid(Combat)) return nullptr; // 무기가 없다면 nullptr 리턴

	return Combat->EquippedWeapon; // 장착된 무기 return
}

FVector ABaseCharacter::GetHitTarget() const
{
	if (false == IsValid(Combat)) return FVector(); //nullptr면 빈 FVector 리턴

	return Combat->HitTarget; // CombatComponent의 HitTaget을 return
}

ECombatState ABaseCharacter::GetCombatState() const
{
	if (false == IsValid(Combat)) return ECombatState::ECS_MAX;

	return Combat->CombatState;
}

bool ABaseCharacter::IsLocallyReloading()
{
	if (false == IsValid(Combat)) return false;

	return Combat->bLocallyReloading;
}

bool ABaseCharacter::IsHoldingTheFlag() const
{
	if (false == IsValid(Combat)) return false;

	return Combat->bHoldingTheFlag;
}

ETeam ABaseCharacter::GetTeam()
{
	MultiplayerPlayerState = MultiplayerPlayerState == nullptr ? GetPlayerState<AMultiplayerPlayerState>() : MultiplayerPlayerState;
	if (MultiplayerPlayerState == nullptr) return ETeam::ET_NoTeam;

	return MultiplayerPlayerState->GetTeam();
}

void ABaseCharacter::SetHoldingTheFlag(bool bHolding)
{
	if (false == IsValid(Combat)) return;

	Combat->bHoldingTheFlag = bHolding;
}