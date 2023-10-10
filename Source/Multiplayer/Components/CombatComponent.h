#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Multiplayer/HUD/MainHUD.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.0f

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

protected:
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable) //Server RPC
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FireButtonPressed(bool bPressed); // 총 발사 버튼 Pressed
	void Fire();

	UFUNCTION(Server, Reliable) // Server RPC 총 발사
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable) // 호출하는 Client에서 실행하는 총 발사 함수
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

private:
	TWeakObjectPtr<class ABaseCharacter> Character;
	TWeakObjectPtr<class AMainPlayerController> Controller;
	TWeakObjectPtr<class AMainHUD> HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon) //서버에 알린다
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

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
	int32 StartingARAmmo = 30; // 게임 시작 시 CarriedAmmo 기본값

	void InitializeCarriedAmmo(); // 게임 시작 시 CarriedAmmo 설정

public:
		
};
