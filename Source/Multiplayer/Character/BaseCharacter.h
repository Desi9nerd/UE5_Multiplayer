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
	//** Play 몽타주
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();
	virtual void OnRep_ReplicatedMovement() override;
	void Elim(); // Server에서만 콜되는 Elim함수

	UFUNCTION(NetMulticast, Reliable) // RPC
	void MulticastElim(); // Player 삭제

	virtual void Destroyed() override;

	UPROPERTY(Replicated) // Client들에게도 알려줄 수 있도록 Replicated
	bool bDisableGameplay = false; // Gameplay 관련 기능들을 불활성화시켜주려 만든 true/false 변수. true면 캐릭터 움직임 제한, but 마우스 회전가능
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth(); // 체력 Update
	void UpdateHUDShield(); // 실드 Update
	void UpdateHUDAmmo();	// 총알 Update
	void SpawnDefaultWeapon(); // 게임시작 시 기본무기 Spawn

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
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);// 피격 데미지
	void PollInit(); // HUD와 점수/승리횟수 매기기 관련 클래스 초기화
	void RotateInPlace(float DeltaTime);


private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraSpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	//OnRep_OverlappingWeapon 함수가 client에 호출되었을때 Replicate해준다.
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

	// Reliable RPC는 무조건 실행, Unreliable RPC는 패킷 Drop 가능
	UFUNCTION(Server, Reliable) // Server를 붙여 함수가 Client에서 호출되지만 Server에서 실행되는 RPC 로 선언
	void ServerEquipButtonPressed();

	float AO_Yaw; // AO에 사용할 Yaw값
	float InterpAO_Yaw;
	float AO_Pitch; // AO에 사용할 Pitch값
	FRotator StartingAimRotation; //마우스 움직임에 따른 회전값

	//** 몽타주
	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage; // 발사 몽타주
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage; // 재장전 몽타주
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage; // 피격 몽타주
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage; // 사망 몽타주
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage; // 수류탄 투척 몽타주
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* SwapMontage; // 무기 교체 몽타주

	void HideCameraIfCharacterClose(); //카메라와 캐릭터가 지나치게 가까워지면 캐릭터를 숨겨준다.

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.0f;

	bool bRotateRootBone; 
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;//마지막 움직임이 Replicated된 후 경과한 시간
	float CalculateSpeed();

	//** 체력 Health
	UPROPERTY(EditAnywhere, Category = "Player Attributes")
	float MaxHealth = 100.0f;
	//ReplicatedUsing으로 사용하려면 GetLifetimeReplicatedProps에 등록해야 한다. 
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Attributes")
	float Health = 100.0f; 
	UFUNCTION()
	void OnRep_Health(float LastHealth); // 캐릭터 체력 변화

	//** 실드 Shield
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
	float ElimDelay = 3.0f; // 죽은 Player가 사라지는데 걸리는 시간

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
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance; // 런타임에 변경되는 Dynamic Instance. 
	
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;// Material Instance. BP에서 등록

	//** Elim Bot
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect; 

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	TWeakObjectPtr<class AMultiplayerPlayerState> MultiplayerPlayerState;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade; // 수류탄 매쉬

	//** 게임시작 시 지급되는 기본 무기
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();//조준 중인지 true/false리턴하는 함수

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; } // TurningInPlace Enum값을 return
	FVector GetHitTarget() const; // CombatComponent의 HitTaget을 return하는 함수
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
