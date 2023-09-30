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

private:
	class ABaseCharacter* Character;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon) //서버에 알린다
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed; // Aiming(X) 캐릭터 Walk이동속도 

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed; // Aiming(O) 캐릭터 Walk이동속도

	bool bFireButtonPressed;

	UFUNCTION(Server, Reliable) // Server RPC 총 발사
	void ServerFire();

	UFUNCTION(NetMulticast, Reliable) // 호출하는 Client에서 실행하는 총 발사 함수
	void MulticastFire(); 

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	FVector HitTarget; //발사체의 충돌지점(=Crosshair위치에서 쏜 linetrace의 충돌지점)

public:
		
};
