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
enum class EWeaponState : uint8 //���� ���� Enum
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType : uint8 // �߻� ����
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
	virtual void OnRep_Owner() override; // AActor�� �Լ� �������̵�
	void SetHUDAmmo();
	void SetHUDWeaponImg();
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	virtual void Dropped(); // �Ҹ� �� ���� ����߸���
	void AddAmmo(int32 AmmoToAdd);
	FVector TraceEndWithScatter(const FVector& HitTarget); // ��ź������ ���� LineTrace�� End Loc ���� �����ϴ� �Լ�
	void EnableCustomDepth(bool bEnable); // Custom Depth ���� true/false

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

	//** ���� �� Zoom FOV
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.0f;
	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.0f; // Zoom ��ȯ �ð�����

	//** �ڵ� �߻� Automatic fire	
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.15f; // �߻� �� ���� �ð�. �߻��� �ð�
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true; // �ڵ� �߻繫�� true/false

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> EquipSound; // �������� ����

	bool bDestroyWeapon = false;

	UPROPERTY(EditAnywhere)
	EFireType FireType; // �߻� ����

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false; // ��ź���� true/false

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
	float DistanceToSphere = 800.f; // ������ SphereRadius������ �Ÿ�
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f; // ������ ��ź������ �̿�� SphereRadius

	UPROPERTY(EditAnywhere)
	float Damage = 20.f; // ���� ������
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 60.f; // ���� ��弦 ������

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false; // ServerSideRewind ��� true/false

	UPROPERTY()
	TObjectPtr<ABaseCharacter> BaseCharcterOwnerCharacter;
	UPROPERTY()
	TObjectPtr<AMainPlayerController> MainPlayerOwnerController;
	
private:
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	void SpendRound(); // �Ѿ�(=Ammo) �Ҹ� �� HUD ������Ʈ

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState; //���� ����

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<UWidgetComponent> PickupWidget; //�����ݱ� Widget(Press E-PickUp)

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TObjectPtr<UAnimationAsset> FireAnimation; //���� Animation

	UPROPERTY(EditAnywhere)
	TSubclassOf<ACasing> CasingClass; //�Ѿ� ź�� class

	UPROPERTY(EditAnywhere) // �Ѿ��� �� �̻� Replicate ���� �ʴ´�.
	int32 Ammo; // ���� �Ѿ� ��

	UPROPERTY(EditAnywhere)
	int32 MagCapacity; // ���� ź������ �� �� �ִ� �Ѿ� �ִ밪

	// The number of unprocessed server requests for Ammo.
	// Incremented in SpendRound, decremented in ClientUpdateAmmo.
	int32 Sequence = 0;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType; // ���� ����

	UPROPERTY(EditAnywhere)
	ETeam Team;
};