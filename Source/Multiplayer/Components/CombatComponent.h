#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Multiplayer/HUD/MainHUD.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
#include "Multiplayer/EnumTypes/ECombatState.h"
#include "CombatComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MULTIPLAYER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();
	friend class ABaseCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(class AWeapon* WeaponToEquip);
	void SwapWeapons(); // 무기 교체
	void FireButtonPressed(bool bPressed); // 총 발사 버튼 Pressed
	void Reload();

	//** ABP_BaseCharacter의 Event Graph에서 BP로 쓰임
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	UFUNCTION(BlueprintCallable)
	void FinishSwap();
	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapons();
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();
	void JumpToShotgunEnd();
	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();
	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

	UFUNCTION(Server, Reliable) // Server RPC
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount); // 무기 줍기

	bool bLocallyReloading = false;

protected:
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable) //Server RPC
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	void Fire();
	void FireProjectileWeapon();
	void FireHitScanWeapon();
	void FireShotgun();
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	UFUNCTION(Server, Reliable, WithValidation) // Server RPC 총 발사
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, float FireDelay);
	UFUNCTION(NetMulticast, Reliable) // 호출하는 Client에서 실행하는 총 발사 함수
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	UFUNCTION(Server, Reliable, WithValidation) // Server RPC
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay);
	UFUNCTION(NetMulticast, Reliable) // 호출하는 Client에서 실행하는 총 발사 함수
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable) // Server RPC
	void ServerReload();

	void HandleReload(); // Server와 Client들 모두에서 실행되는 재장전 몽타주 재생 함수
	int32 AmountToReload(); // 재장전하는 총알 수를 리턴하는 함수

	void ThrowGrenade(); // 수류탄 투척. Client
	UFUNCTION(Server, Reliable) // Server RPC
	void ServerThrowGrenade();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	void DropEquippedWeapon(); // 장착중인 무기 떨어뜨리기
	void AttachActorToRightHand(AActor* ActorToAttach); // 무기 오른손 소켓에 붙이기
	void AttachActorToLeftHand(AActor* ActorToAttach); // 무기 왼손 소켓에 붙이기
	void AttachActorToBackpack(AActor* ActorToAttach); // 무기 배낭위치에 붙이기
	void UpdateCarriedAmmo(); // 현재 장착무기 탄창의 최대 총알 수 업데이트
	void PlayEquipWeaponSound(AWeapon* WeaponToEquip); // 무기 장착 사운드 재생
	void ReloadEmptyWeapon(); // 총알이 비었는지 확인하고 만약 비었을 시 재장전
	void ShowAttachedGrenade(bool bShowGrenade); // 캐릭터 손에 수류탄이 보일지 말지 결정
	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

private:
	TWeakObjectPtr<class ABaseCharacter> Character;
	TWeakObjectPtr<class AMainPlayerController> Controller;
	TWeakObjectPtr<class AMainHUD> HUD;

	//서버에 알린다. 그 후 모든 Client에 알린다.
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon) 
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming = false; // 조준 
	bool bAimButtonPressed = false; // 조준버튼눌림 
	UFUNCTION()
	void OnRep_Aiming();

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed; // Aiming(X) 캐릭터 Walk이동속도 
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed; // Aiming(O) 캐릭터 Walk이동속도

	bool bFireButtonPressed;

	//** HUD + Crosshair, Crosshair 모아지고 퍼지게하기 구현
	float CrosshairVelocityFactor;// 캐릭터 이동으로 Crosshair 퍼짐에 영향을 주는 정도
	float CrosshairInAirFactor;// 캐릭터가 공중에 있을때 Crosshair 퍼짐에 영향을 주는 정도
	float CrosshairAimFactor; // 조준 할 때
	float CrosshairShootingFactor; // 발사 할 때

	FVector HitTarget; // 총알이 발사되서 충돌하게 될 지점
	FHUDPackage HUDPackage;

	//** 조준(Aiming) & FOV
	float DefaultFOV; // FOV 기본값. 카메라의 기본 FOV값(=Aiming(X) 일 때 FOV 값)

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.0f; //조준(Aiming) 시 FOV 값

	float CurrentFOV; // 현재 FOV 값

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.0f; // FOV 전환 시간간격

	void InterpFOV(float DeltaTime);

	//** 자동 발사 Automatic Fire
	FTimerHandle FireTimer; // Automatic Fire 타이머 핸들러
	bool bCanFire = true; // 총알 발사 여부가 가능한지 정하는 true/false 변수

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire(); // 총알 발사가 가능한지 true/false 리턴하는 함수

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo; // 현재 장착무기 탄창의 최대 총알 수

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap; // 무기별 탄창 최대 총알 수 Map
	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500; // 모든 무기의 최대 Ammo 개수
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30; // 게임 시작 시 CarriedAmmo 기본값
	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 3; // 게임 시작 시  기본값
	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0; // 게임 시작 시 권총 총알 기본값
	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;
	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 0;
	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 0;
	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 0;

	void InitializeCarriedAmmo(); // 게임 시작 시 CarriedAmmo 설정

	// Server에서 변경하면 Replicate 해준다. 모든 Client들은 CombatState을 알아야 한다.
	UPROPERTY(ReplicatedUsing = OnRep_CombatState) 
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues(); // 총알 수 업데이트
	void UpdateShotgunAmmoValues();

	UPROPERTY(ReplicatedUsing = OnRep_Grenades) // Replicated 되는 변수로 설정
	int32 Grenades = 4;

	UFUNCTION() // Rep Notify
	void OnRep_Grenades(); 

	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;

	void UpdateHUDGrenades();

	bool bHoldingTheFlag = false; // 점령전 깃발 들고있는지 true/false

public:
	bool ShouldSwapWeapons() { return (EquippedWeapon != nullptr && SecondaryWeapon != nullptr); };
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
};