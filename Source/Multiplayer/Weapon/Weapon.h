#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
#include "Weapon.generated.h"

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
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override; // AActor의 함수 오버라이드
	void SetHUDAmmo();
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	void Dropped(); // 소멸 후 무기 떨어뜨리기
	void AddAmmo(int32 AmmoToAdd);
	FVector TraceEndWithScatter(const FVector& HitTarget); // 산탄분포를 위해 LineTrace의 End Loc 랜덤 변경하는 함수

	//** Crosshair Texture
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairBottom;

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
	class USoundCue* EquipSound; // 무기장착 사운드
	
	void EnableCustomDepth(bool bEnable); // Custom Depth 적용 true/false

	bool bDestroyWeapon = false;

	UPROPERTY(EditAnywhere)
	EFireType FireType; // 발사 종류

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false; // 샷건의 산탄분포 true/false

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

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState; //무기 상태

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget; //무기줍기 Widget(Press E-PickUp)

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation; //무기 Animation

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass; //총알 탄피 class

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo; // 현재 총알 수

	void SpendRound();

	UFUNCTION()
	void OnRep_Ammo();

	UPROPERTY(EditAnywhere)
	int32 MagCapacity; // 무기 탄알집에 들어갈 수 있는 총알 최대값

	UPROPERTY()
	class ABaseCharacter* BaseCharcterOwnerCharacter;
	UPROPERTY()
	class AMainPlayerController* MainPlayerOwnerController;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType; // 무기 종류

	//** Trace end with scatter
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.0f; // 샷건의 SphereRadius까지의 거리
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.0f; // 샷건의 산탄분포에 이용될 SphereRadius

public:
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	bool IsEmpty(); // 총알이 없는지 확인하는 함수. Ammo<=0면 true 리턴.
	bool IsFull();
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
};