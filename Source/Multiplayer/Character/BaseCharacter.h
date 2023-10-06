#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Multiplayer/EnumTypes/ETurningInPlace.h"
#include "Multiplayer/Interfaces/ICrosshair.h"
#include "BaseCharacter.generated.h"

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
	void PlayFireMontage(bool bAiming); 
	void PlayElimMontage();
	virtual void OnRep_ReplicatedMovement() override;

	UFUNCTION(NetMulticast, Reliable) // RPC
	void Elim(); // Player 삭제

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);// 피격 데미지
	void UpdateHUDHealth(); // 체력 Update

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

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	//Reliable RPC는 무조건 실행, Unreliable RPC는 패킷 Drop 가능
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw; // AO에 사용할 Yaw값
	float InterpAO_Yaw;
	float AO_Pitch; // AO에 사용할 Pitch값
	FRotator StartingAimRotation; //마우스 움직임에 따른 회전값

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage; // 발사 몽타주

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage; // 피격 몽타주

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

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
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Attribute")
	float Health = 100.0f; 

	UFUNCTION()
	void OnRep_Health();
	
	TObjectPtr<class AMainPlayerController> MainPlayerController;

	bool bElimmed = false;

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
};
