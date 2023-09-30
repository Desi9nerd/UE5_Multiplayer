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

	void FireButtonPressed(bool bPressed); // �� �߻� ��ư Pressed

private:
	class ABaseCharacter* Character;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon) //������ �˸���
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed; // Aiming(X) ĳ���� Walk�̵��ӵ� 

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed; // Aiming(O) ĳ���� Walk�̵��ӵ�

	bool bFireButtonPressed;

	UFUNCTION(Server, Reliable) // Server RPC �� �߻�
	void ServerFire();

	UFUNCTION(NetMulticast, Reliable) // ȣ���ϴ� Client���� �����ϴ� �� �߻� �Լ�
	void MulticastFire(); 

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	FVector HitTarget; //�߻�ü�� �浹����(=Crosshair��ġ���� �� linetrace�� �浹����)

public:
		
};
