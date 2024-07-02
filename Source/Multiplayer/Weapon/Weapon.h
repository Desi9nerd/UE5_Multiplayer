#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
#include "Multiplayer/EnumTypes/ETeam.h"
#include "Weapon.generated.h"

class AMainPlayerController;
class ABaseCharacter;
class USoundCue;
class UWidgetComponent;
class USphereComponent;
class ACasing;

UENUM(BlueprintType)
enum class EWeaponState : uint8 //무기 상태 Enum
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType : uint8 // 발사 종류
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class MULTIPLAYER_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE UWidgetComponent* GetPickupWidget() const { return PickupWidget; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
	FORCEINLINE ETeam GetTeam() const { return Team; }
	bool IsEmpty() { return Ammo <= 0; }
	bool IsFull() { return Ammo == MagCapacity; }

	AWeapon();
	void SetWeaponState(EWeaponState State);

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override; // AActor의 함수 오버라이드
	void SetHUDAmmo();
	void SetHUDWeaponImg();
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	virtual void Dropped(); // 소멸 후 무기 떨어뜨리기
	void AddAmmo(int32 AmmoToAdd);
	FVector TraceEndWithScatter(const FVector& HitTarget); // 산탄분포를 위해 LineTrace의 End Loc 랜덤 변경하는 함수
	void EnableCustomDepth(bool bEnable); // Custom Depth 적용 true/false

	//** Crosshair Texture
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	TObjectPtr<UTexture2D> CrosshairCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	TObjectPtr<UTexture2D> CrosshairLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	TObjectPtr<UTexture2D> CrosshairRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	TObjectPtr<UTexture2D> CrosshairTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	TObjectPtr<UTexture2D> CrosshairBottom;

	//** 조준 중 Zoom FOV
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.0f;
	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.0f; // Zoom 전환 시간간격

	//** 자동 발사 Automatic fire	
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.15f; // 발사 간 간격 시간. 발사대기 시간
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true; // 자동 발사무기 true/false

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> EquipSound; // 무기장착 사운드

	bool bDestroyWeapon = false;

	UPROPERTY(EditAnywhere)
	EFireType FireType; // 발사 종류

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false; // 산탄분포 true/false

protected:
	virtual void BeginPlay() override;
	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnDropped();
	virtual void OnEquippedSecondary();

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,	const FHitResult& SweepResult);
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,	int32 OtherBodyIndex);

	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);

	//** Trace end with scatter
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f; // 샷건의 SphereRadius까지의 거리
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f; // 샷건의 산탄분포에 이용될 SphereRadius

	UPROPERTY(EditAnywhere)
	float Damage = 20.f; // 무기 데미지
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 60.f; // 무기 헤드샷 데미지

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false; // ServerSideRewind 사용 true/false

	UPROPERTY()
	TObjectPtr<ABaseCharacter> BaseCharcterOwnerCharacter;
	UPROPERTY()
	TObjectPtr<AMainPlayerController> MainPlayerOwnerController;
	
private:
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	void SpendRound(); // 총알(=Ammo) 소모 후 HUD 업데이트

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState; //무기 상태

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<UWidgetComponent> PickupWidget; //무기줍기 Widget(Press E-PickUp)

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation; //무기 Animation

	UPROPERTY(EditAnywhere)
	TSubclassOf<ACasing> CasingClass; //총알 탄피 class

	UPROPERTY(EditAnywhere) // 총알은 더 이상 Replicate 하지 않는다.
	int32 Ammo; // 현재 총알 수

	UPROPERTY(EditAnywhere)
	int32 MagCapacity; // 무기 탄알집에 들어갈 수 있는 총알 최대값

	// The number of unprocessed server requests for Ammo.
	// Incremented in SpendRound, decremented in ClientUpdateAmmo.
	int32 Sequence = 0;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType; // 무기 종류

	UPROPERTY(EditAnywhere)
	ETeam Team;
};