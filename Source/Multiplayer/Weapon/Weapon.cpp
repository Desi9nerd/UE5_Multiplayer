#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Multiplayer/Components/CombatComponent.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; //만약 false라면 replicated되지 않아 모든 플레이어가 Authority를 가지게 된다. 예를 들어 플레이어1이 총을 발사하면 다른 플레이어들도 (각각의 독립된 서버가 존재하기 때문에) 총이 발사된다. 하지만 bReplicates=true라면 서버 하나만 Authority를 가지고 관리한다. 플레이어1이 총을 발사하면 서버에 정보를 알리고 모든 Client에 전파한다.
	SetReplicateMovement(true); // 무기의 Movement를 Replicated 해준다.

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);//WeaponMesh의 channel 충돌을 block 처리
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);//Pawn의 경우, 충돌 무시, 무기가 Pawn(ex.적 캐릭터)에 부딪혔을때 통과할 수 있도록 설정.
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);//처음 무기가 나왔을때 No Collision

	EnableCustomDepth(true); // Custom Depth가 가능한 상태를 초기값으로 설정
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);
	WeaponMesh->MarkRenderStateDirty();

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);//channel 충돌x, AreaSphere는 충돌x
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState); // WeaponState를 모든 Client들에게 Replicate 해준다.
	DOREPLIFETIME(AWeapon, Ammo); // Ammo를 모든 Client들에게 Replicate 해준다.
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (IsValid(WeaponMesh))
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	//서버가 Weapon 객체를 관리할 수 있도록 한다
	if (HasAuthority()) //HasAuthority()와 GetLocalRole() == ENetRole::ROLE_Authority는 같다.
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap); //무기 AreaSphere와 겹치면 OnSphereOverlap()함수 Delegate호출
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap); //무기 AreaSphere와 겹친게 해제되면 OnSphereEndOverlap()함수 Delegate호출
	}
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false); //PickupWidget을 꺼주고 시작한다.
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter.IsValid())
	{
		BaseCharacter->SetOverlappingWeapon(this);//캐릭터와 무기AreaSphere이 겹치면 무기정보 넘김
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter.IsValid())
	{
		BaseCharacter->SetOverlappingWeapon(nullptr);//캐릭터와 무기AreaSphere이 안 겹치면 nullptr
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State; //무기상태 변경
	OnWeaponStateSet();

}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped: // Primary 무기로 장착된 상태
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary: // Secondary 무기로 장착된 상태
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped: // 무기가 바닥에 떨어져 있는 상태
		OnDropped();
		break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnEquipped() //무기 장착상태
{
	ShowPickupWidget(false); // PickupWidget 꺼줌
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); // AreaSphere 충돌X
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);// WeaponMesh. 충돌X.
	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 끈 부분 Physics 적용
		WeaponMesh->SetEnableGravity(true); // 중력을 켜준다.
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
}

void AWeapon::OnDropped() // 무기가 바닥에 떨어져 있는 상태
{
	if (HasAuthority()) // Server
	{	
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // AreaSphere 충돌O
	}
	WeaponMesh->SetSimulatePhysics(true); // 물리법칙O, 만약 false면 Gravity와 같은 물리법칙들이 적용X
	WeaponMesh->SetEnableGravity(true); // 중력O
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 충돌O
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	//** Custom Depth 적용O
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
}

void AWeapon::OnEquippedSecondary() // Secondary 무기로 장착된 상태
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 끈 부분 Physics 적용
		WeaponMesh->SetEnableGravity(true); // 중력을 켜준다.
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
	//** Secondary 무기에는 윤곽선 효과 적용. 테스트 후 원하지 않을시 삭제해도됨
	//EnableCustomDepth(true);
	//if (IsValid(WeaponMesh))
	//{
	//	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
	//	WeaponMesh->MarkRenderStateDirty();
	//}	
}

void AWeapon::SetHUDAmmo()
{
	BaseCharcterOwnerCharacter = BaseCharcterOwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : BaseCharcterOwnerCharacter;
	if (IsValid(BaseCharcterOwnerCharacter))
	{
		MainPlayerOwnerController = MainPlayerOwnerController == nullptr ? Cast<AMainPlayerController>(BaseCharcterOwnerCharacter->Controller) : MainPlayerOwnerController;
		if (IsValid(MainPlayerOwnerController))
		{
			MainPlayerOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::SpendRound() 
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity); // 총알 소모 -1, 0~MagCapcity 값을 벗어나지 않도록 설정.
	SetHUDAmmo(); // HUD에 총알(Ammo) 수 업데이트
}

void AWeapon::OnRep_Ammo() // Client
{
	BaseCharcterOwnerCharacter = BaseCharcterOwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : BaseCharcterOwnerCharacter;
	// BaseCharcterOwnerCharacter와 CombatComponent가 있고 총알(=탄)이 최대 총알 수이면 
	if (BaseCharcterOwnerCharacter && BaseCharcterOwnerCharacter->GetCombat() && IsFull())
	{
		BaseCharcterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}

	SetHUDAmmo(); // HUD에 총알(Ammo) 수 업데이트
}

void AWeapon::OnRep_Owner() // Client
{
	Super::OnRep_Owner(); // AActor에 정의된 함수 Super

	if (Owner == nullptr)
	{
		BaseCharcterOwnerCharacter = nullptr;
		MainPlayerOwnerController = nullptr;
	}
	else
	{
		BaseCharcterOwnerCharacter = BaseCharcterOwnerCharacter == nullptr ? Cast<ABaseCharacter>(Owner) : BaseCharcterOwnerCharacter;
		// BaseCharcterOwnerCharacter와 장착된 무기가 있고 장착된 무기가 현재무기(=this)라면 
		if (BaseCharcterOwnerCharacter && BaseCharcterOwnerCharacter->GetEquippedWeapon() && BaseCharcterOwnerCharacter->GetEquippedWeapon() == this)
		{
			SetHUDAmmo(); // HUD에 총알(Ammo) 수 업데이트
		}
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (IsValid(FireAnimation))
	{
		WeaponMesh->PlayAnimation(FireAnimation, false); //발사 애니메이션 플레이
	}

	if (IsValid(CasingClass)) //총알 탄피 클래스가 있다면
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject")); //GetSocketTransform을 사용하려면 SkeletalMeshSocket가 필요하다.아래에서 사용하기 위해 변수 생성.

		if (IsValid(AmmoEjectSocket)) // "AmmoEject"소켓이 있다면
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);// Spawn위치로 쓸 FTransform 변수

			UWorld* World = GetWorld();
			if (IsValid(World))
			{
				// 월드에 총알탄피(CasingClass)를 SocketTransform위치에 Spawn 시킨다.
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
					); 
			}
		}

		SpendRound(); // 총알 소모 -1
	}
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped); // 무기상태 Dropped로 변경
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);//소멸 시 World에 무기가 떨어지도록 DetachmentTransformRules 설정
	WeaponMesh->DetachFromComponent(DetachRules); // WeaponMesh를 캐릭터에서 떨어뜨린다.
	SetOwner(nullptr); // 캐릭터가 무기를 떨어뜨린 후 무기의 Owner가 없도록 Owner를 nullptr로 설정

	BaseCharcterOwnerCharacter = nullptr; // 무기소유 캐릭터가 없도록 nullptr
	MainPlayerOwnerController = nullptr; // 무기소유 controller가 없도록 nullptr
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo - AmmoToAdd, 0, MagCapacity); // 최소값 0, 최대값 MagCapcity
	SetHUDAmmo();
}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}

bool AWeapon::IsFull()
{
	return Ammo == MagCapacity;
}
