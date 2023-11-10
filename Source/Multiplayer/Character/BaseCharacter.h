#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Multiplayer/EnumTypes/ETurningInPlace.h"
#include "Multiplayer/Interfaces/ICrosshair.h"
#include "Components/TimelineComponent.h"
#include "Multiplayer/EnumTypes/ECombatState.h"
#include "BaseCharacter.generated.h"

class AWeapon;

UCLASS()
class MULTIPLAYER_API ABaseCharacter
	: public ACharacter, public IICrosshair
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	//** Play ��Ÿ��
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();
	virtual void OnRep_ReplicatedMovement() override;
	void Elim(); // Server������ �ݵǴ� Elim�Լ�

	UFUNCTION(NetMulticast, Reliable) // RPC
	void MulticastElim(); // Player ����

	virtual void Destroyed() override;

	UPROPERTY(Replicated) // Client�鿡�Ե� �˷��� �� �ֵ��� Replicated
	bool bDisableGameplay = false; // Gameplay ���� ��ɵ��� ��Ȱ��ȭ�����ַ� ���� true/false ����. true�� ĳ���� ������ ����, but ���콺 ȸ������
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth(); // ü�� Update
	void UpdateHUDShield(); // �ǵ� Update
	void UpdateHUDAmmo();	// �Ѿ� Update
	void SpawnDefaultWeapon(); // ���ӽ��� �� �⺻���� Spawn

	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;

	bool bFinishedSwapping = false;

protected:
	virtual void BeginPlay() override;
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void ReloadButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();
	void GrenadeButtonPressed();
	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);// �ǰ� ������
	void PollInit(); // HUD�� ����/�¸�Ƚ�� �ű�� ���� Ŭ���� �ʱ�ȭ
	void RotateInPlace(float DeltaTime);


private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraSpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	//OnRep_OverlappingWeapon �Լ��� client�� ȣ��Ǿ����� Replicate���ش�.
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)  
	class AWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* LagCompensation;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	// Reliable RPC�� ������ ����, Unreliable RPC�� ��Ŷ Drop ����
	UFUNCTION(Server, Reliable) // Server�� �ٿ� �Լ��� Client���� ȣ������� Server���� ����Ǵ� RPC �� ����
	void ServerEquipButtonPressed();

	float AO_Yaw; // AO�� ����� Yaw��
	float InterpAO_Yaw;
	float AO_Pitch; // AO�� ����� Pitch��
	FRotator StartingAimRotation; //���콺 �����ӿ� ���� ȸ����

	//** ��Ÿ��
	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage; // �߻� ��Ÿ��
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage; // ������ ��Ÿ��
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage; // �ǰ� ��Ÿ��
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage; // ��� ��Ÿ��
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage; // ����ź ��ô ��Ÿ��
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* SwapMontage; // ���� ��ü ��Ÿ��

	void HideCameraIfCharacterClose(); //ī�޶�� ĳ���Ͱ� ����ġ�� ��������� ĳ���͸� �����ش�.

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.0f;

	bool bRotateRootBone; 
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;//������ �������� Replicated�� �� ����� �ð�
	float CalculateSpeed();

	//** ü�� Health
	UPROPERTY(EditAnywhere, Category = "Player Attributes")
	float MaxHealth = 100.0f;
	//ReplicatedUsing���� ����Ϸ��� GetLifetimeReplicatedProps�� ����ؾ� �Ѵ�. 
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Attributes")
	float Health = 100.0f; 
	UFUNCTION()
	void OnRep_Health(float LastHealth); // ĳ���� ü�� ��ȭ

	//** �ǵ� Shield
	UPROPERTY(EditAnywhere, Category = "Player Attributes")
	float MaxShield = 100.0f;
	UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player Attributes")
	float Shield = 0.0f;
	UFUNCTION()
	void OnRep_Shield(float LastShield);

	UPROPERTY()
	class AMainPlayerController* MainPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.0f; // ���� Player�� ������µ� �ɸ��� �ð�

	void ElimTimerFinished();

	//** Dissolve effect
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();
	
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance; // ��Ÿ�ӿ� ����Ǵ� Dynamic Instance. 
	
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;// Material Instance. BP���� ���

	//** Elim Bot
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect; 

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	TWeakObjectPtr<class AMultiplayerPlayerState> MultiplayerPlayerState;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade; // ����ź �Ž�

	//** ���ӽ��� �� ���޵Ǵ� �⺻ ����
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();//���� ������ true/false�����ϴ� �Լ�

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; } // TurningInPlace Enum���� return
	FVector GetHitTarget() const; // CombatComponent�� HitTaget�� return�ϴ� �Լ�
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	ECombatState GetCombatState() const;
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	bool IsLocallyReloading();
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }

protected:
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* BackpackMesh;

	//** Hit Boxes used for Server-side Rewind
	UPROPERTY(EditAnywhere)
	class UBoxComponent* head;
	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;
	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* backpack;
	UPROPERTY(EditAnywhere)
	UBoxComponent* blanket;
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;
};
