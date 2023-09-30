#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; //만약 false라면 replicated되지 않아 모든 플레이어가 Authority를 가지게 된다. 예를 들어 플레이어1이 총을 발사하면 다른 플레이어들도 (각각의 독립된 서버가 존재하기 때문에) 총이 발사된다. 하지만 bReplicates=true라면 서버 하나만 Authority를 가지고 관리한다. 플레이어1이 총을 발사하면 서버에 정보를 알리고 모든 Client에 전파한다.

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);//WeaponMesh의 channel 충돌을 block 처리
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);//Pawn의 경우, 충돌 무시, 무기가 Pawn(ex.적 캐릭터)에 부딪혔을때 통과할 수 있도록 설정.
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);//처음 무기가 나왔을때 No Collision

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

	DOREPLIFETIME(AWeapon, WeaponState);
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
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

	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped: //무기 장착상태 시
		ShowPickupWidget(false); //PickupWidget 꺼줌
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);//AreaSphere충돌x
		break;
	}	
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped: //무기 장착상태 시
		ShowPickupWidget(false); //PickupWidget 꺼줌
		break;
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (IsValid(FireAnimation))
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
}