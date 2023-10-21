#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Multiplayer/HUD/MainHUD.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
#include "Multiplayer/EnumTypes/ECombatState.h"
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
	void FireButtonPressed(bool bPressed); // �� �߻� ��ư Pressed
	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	
protected:
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable) //Server RPC
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void Fire();

	UFUNCTION(Server, Reliable) // Server RPC �� �߻�
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable) // ȣ���ϴ� Client���� �����ϴ� �� �߻� �Լ�
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable) // Server RPC
	void ServerReload();

	void HandleReload(); // Server�� Client�� ��ο��� ����Ǵ� ������ ��Ÿ�� ��� �Լ�
	int32 AmountToReload(); // �������ϴ� �Ѿ� ���� �����ϴ� �Լ�

private:
	TWeakObjectPtr<class ABaseCharacter> Character;
	TWeakObjectPtr<class AMainPlayerController> Controller;
	TWeakObjectPtr<class AMainHUD> HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon) //������ �˸���
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed; // Aiming(X) ĳ���� Walk�̵��ӵ� 

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed; // Aiming(O) ĳ���� Walk�̵��ӵ�

	bool bFireButtonPressed;

	//** HUD + Crosshair, Crosshair ������� �������ϱ� ����
	float CrosshairVelocityFactor;// ĳ���� �̵����� Crosshair ������ ������ �ִ� ����
	float CrosshairInAirFactor;// ĳ���Ͱ� ���߿� ������ Crosshair ������ ������ �ִ� ����
	float CrosshairAimFactor; // ���� �� ��
	float CrosshairShootingFactor; // �߻� �� ��

	FVector HitTarget; // �Ѿ��� �߻�Ǽ� �浹�ϰ� �� ����
	FHUDPackage HUDPackage;

	//** ����(Aiming) & FOV
	float DefaultFOV; // FOV �⺻��. ī�޶��� �⺻ FOV��(=Aiming(X) �� �� FOV ��)

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.0f; //����(Aiming) �� FOV ��

	float CurrentFOV; // ���� FOV ��

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.0f; // FOV ��ȯ �ð�����

	void InterpFOV(float DeltaTime);

	//** �ڵ� �߻� Automatic Fire
	FTimerHandle FireTimer; // Automatic Fire Ÿ�̸� �ڵ鷯
	bool bCanFire = true; // �Ѿ� �߻� ���ΰ� �������� ���ϴ� true/false ����

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire(); // �Ѿ� �߻簡 �������� true/false �����ϴ� �Լ�

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo; // ���� �������� źâ�� �ִ� �Ѿ� ��

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap; // ���⺰ źâ �ִ� �Ѿ� �� Map

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30; // ���� ���� �� CarriedAmmo �⺻��

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 3; // ���� ���� ��  �⺻��

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0; // ���� ���� �� ���� �Ѿ� �⺻��

	void InitializeCarriedAmmo(); // ���� ���� �� CarriedAmmo ����

	// Server���� �����ϸ� Replicate ���ش�. ��� Client���� CombatState�� �˾ƾ� �Ѵ�.
	UPROPERTY(ReplicatedUsing = OnRep_CombatState) 
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues(); // �Ѿ� �� ������Ʈ

public:
		
};