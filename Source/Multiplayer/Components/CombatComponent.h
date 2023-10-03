#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

	//** HUD + Crosshair
	float CrosshairVelocityFactor;// 캐릭터 이동으로 Crosshair 퍼짐에 영향을 주는 정도
	float CrosshairInAirFactor;// 캐릭터가 공중에 있을때 Crosshair 퍼짐에 영향을 주는 정도

	FVector HitTarget; // 총알이 발사되서 충돌하게 될 지점

	//** 조준(Aiming) & FOV
	float DefaultFOV; // FOV 기본값. 카메라의 기본 FOV값(=Aiming(X) 일 때 FOV 값)

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.0f; //조준(Aiming) 시 FOV 값

	float CurrentFOV; // 현재 FOV 값

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.0f; // FOV 전환 시간간격

	void InterpFOV(float DeltaTime);

public:
		
};
