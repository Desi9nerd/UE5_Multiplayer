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
#include "Kismet/KismetMathLibrary.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; //���� false��� replicated���� �ʾ� ��� �÷��̾ Authority�� ������ �ȴ�. ���� ��� �÷��̾�1�� ���� �߻��ϸ� �ٸ� �÷��̾�鵵 (������ ������ ������ �����ϱ� ������) ���� �߻�ȴ�. ������ bReplicates=true��� ���� �ϳ��� Authority�� ������ �����Ѵ�. �÷��̾�1�� ���� �߻��ϸ� ������ ������ �˸��� ��� Client�� �����Ѵ�.
	SetReplicateMovement(true); // ������ Movement�� Replicated ���ش�.

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);//WeaponMesh�� channel �浹�� block ó��
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);//Pawn�� ���, �浹 ����, ���Ⱑ Pawn(ex.�� ĳ����)�� �ε������� ����� �� �ֵ��� ����.
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);//ó�� ���Ⱑ �������� No Collision

	EnableCustomDepth(true); // Custom Depth�� ������ ���¸� �ʱⰪ���� ����
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);
	WeaponMesh->MarkRenderStateDirty();

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);//channel �浹x, AreaSphere�� �浹x
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

	DOREPLIFETIME(AWeapon, WeaponState); // WeaponState�� ��� Client�鿡�� Replicate ���ش�.
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
	
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap); //���� AreaSphere�� ��ġ�� OnSphereOverlap()�Լ� Delegateȣ��
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap); //���� AreaSphere�� ��ģ�� �����Ǹ� OnSphereEndOverlap()�Լ� Delegateȣ��

	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false); //PickupWidget�� ���ְ� �����Ѵ�.
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter.IsValid())
	{
		BaseCharacter->SetOverlappingWeapon(this);//ĳ���Ϳ� ����AreaSphere�� ��ġ�� �������� �ѱ�
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter.IsValid())
	{
		BaseCharacter->SetOverlappingWeapon(nullptr);//ĳ���Ϳ� ����AreaSphere�� �� ��ġ�� nullptr
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State; //������� ����
	OnWeaponStateSet();

}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped: // Primary ����� ������ ����
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary: // Secondary ����� ������ ����
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped: // ���Ⱑ �ٴڿ� ������ �ִ� ����
		OnDropped();
		break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnEquipped() //���� ��������
{
	ShowPickupWidget(false); // PickupWidget ����
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); // AreaSphere �浹X
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);// WeaponMesh. �浹X.
	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // �� �κ� Physics ����
		WeaponMesh->SetEnableGravity(true); // �߷��� ���ش�.
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
}

void AWeapon::OnDropped() // ���Ⱑ �ٴڿ� ������ �ִ� ����
{
	if (HasAuthority()) // Server
	{	
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // AreaSphere �浹O
	}
	WeaponMesh->SetSimulatePhysics(true); // ������ĢO, ���� false�� Gravity�� ���� ������Ģ���� ����X
	WeaponMesh->SetEnableGravity(true); // �߷�O
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // �浹O
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	//** Custom Depth ����O
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
}

void AWeapon::OnEquippedSecondary() // Secondary ����� ������ ����
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // �� �κ� Physics ����
		WeaponMesh->SetEnableGravity(true); // �߷��� ���ش�.
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
	//** Secondary ���⿡�� ������ ȿ�� ����. �׽�Ʈ �� ������ ������ �����ص���
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

void AWeapon::SpendRound() // �Ѿ�(=Ammo) �Ҹ� �� HUD ������Ʈ
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity); // �Ѿ� �Ҹ� -1, 0~MagCapcity ���� ����� �ʵ��� ����.
	SetHUDAmmo(); // HUD�� �Ѿ�(Ammo) �� ������Ʈ

	if (HasAuthority()) // Server
	{
		// Client-side prediction�� ����ϱ� ������ Server->Client�� ������ ���� �� Ammo�� locally ����.
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		++Sequence;
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;

	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;

	SetHUDAmmo();
}

void AWeapon::AddAmmo(int32 AmmoToAdd) // �Ѿ�(=Ammo) ���ϰ� HUD ������Ʈ
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity); // �ּҰ� 0, �ִ밪 MagCapcity

	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd);
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return; // Server�� ��� ����ó��

	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	BaseCharcterOwnerCharacter = BaseCharcterOwnerCharacter == nullptr ? Cast<ABaseCharacter>(GetOwner()) : BaseCharcterOwnerCharacter;
	if (BaseCharcterOwnerCharacter && BaseCharcterOwnerCharacter->GetCombat() && IsFull())
	{
		BaseCharcterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}

	SetHUDAmmo();
}

void AWeapon::OnRep_Owner() // Client
{
	Super::OnRep_Owner(); // AActor�� ���ǵ� �Լ� Super

	if (Owner == nullptr)
	{
		BaseCharcterOwnerCharacter = nullptr;
		MainPlayerOwnerController = nullptr;
	}
	else
	{
		BaseCharcterOwnerCharacter = BaseCharcterOwnerCharacter == nullptr ? Cast<ABaseCharacter>(Owner) : BaseCharcterOwnerCharacter;
		// BaseCharcterOwnerCharacter�� ������ ���Ⱑ �ְ� ������ ���Ⱑ ���繫��(=this)��� 
		if (BaseCharcterOwnerCharacter && BaseCharcterOwnerCharacter->GetEquippedWeapon() && BaseCharcterOwnerCharacter->GetEquippedWeapon() == this)
		{
			SetHUDAmmo(); // HUD�� �Ѿ�(Ammo) �� ������Ʈ
		}
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (IsValid(FireAnimation))
	{
		WeaponMesh->PlayAnimation(FireAnimation, false); //�߻� �ִϸ��̼� �÷���
	}

	if (IsValid(CasingClass)) //�Ѿ� ź�� Ŭ������ �ִٸ�
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject")); //GetSocketTransform�� ����Ϸ��� SkeletalMeshSocket�� �ʿ��ϴ�.�Ʒ����� ����ϱ� ���� ���� ����.

		if (IsValid(AmmoEjectSocket)) // "AmmoEject"������ �ִٸ�
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);// Spawn��ġ�� �� FTransform ����

			UWorld* World = GetWorld();
			if (IsValid(World))
			{
				// ���忡 �Ѿ�ź��(CasingClass)�� SocketTransform��ġ�� Spawn ��Ų��.
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
					); 
			}
		}
	}

	//** Client-side prediction ���. ��� �ӽſ��� SpendRound()�� ���� �� �ְ� �Ѵ�.
	SpendRound(); // �Ѿ� �Ҹ� -1	
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped); // ������� Dropped�� ����
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);//�Ҹ� �� World�� ���Ⱑ ���������� DetachmentTransformRules ����
	WeaponMesh->DetachFromComponent(DetachRules); // WeaponMesh�� ĳ���Ϳ��� ����߸���.
	SetOwner(nullptr); // ĳ���Ͱ� ���⸦ ����߸� �� ������ Owner�� ������ Owner�� nullptr�� ����

	BaseCharcterOwnerCharacter = nullptr; // ������� ĳ���Ͱ� ������ nullptr
	MainPlayerOwnerController = nullptr; // ������� controller�� ������ nullptr
}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}

bool AWeapon::IsFull()
{
	return Ammo == MagCapacity;
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget) // ��ź������ ���� LineTrace�� End Loc ���� �����ϴ� �Լ�
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return FVector(); // MuzzleFlash ������ ������ �� FVector ����

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation(); // LineTrace ������(=MuzzleFlash ������ġ)

	//** LineTrace�� ������ġ(=TraceStart)�� ����ġ(=HitTarget) ���. ��ź�� �����ϱ� ���� LineTrace�� �������� ������ �������� end location�� ����ǵ��� �� �� �����Ѵ�. 
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart;

	//** LineTrace ������
	//DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	//DrawDebugSphere(GetWorld(), EndLoc, 4.0f, 12, FColor::Orange, true);
	//DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()), FColor::Cyan,true);

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()); // LineTrace�� end location�� ����.
}
