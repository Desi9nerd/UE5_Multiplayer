#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MULTIPLAYER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();
	friend class ABaseCharacter; // BaseCharacter 클래스를 friend 클래스로 설정.

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Heal(float HealAmount, float HealingTime);
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed); // Speed 초기값
	void SetInitialJumpVelocity(float Velocity); // Jump속도 초기값
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime); // Speed Buff 적용
	void BuffJump(float BuffJumpVelocity, float BuffTime);  // Jump Buff 적용

protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);

private:
	TWeakObjectPtr<class ABaseCharacter> Character;

	//** Health Buff
	bool bHealing = false; // 회복 true/false
	float HealingRate = 0; // 회복되는 속도
	float AmountToHeal = 0.0f; // 체력 회복수치

	//** Speed Buff
	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds(); // Speed를 원래대로 되돌리는 함수
	float InitialBaseSpeed;
	float InitialCrouchSpeed;
	UFUNCTION(NetMulticast, Reliable) // Client RPC
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed); // Speed 변경을 알라주는 함수

	//** Jump Buff
	FTimerHandle JumpBuffTimer;
	void ResetJump(); // Jump속도값을 원래대로 되돌리는 함수
	float InitialJumpVelocity;
	UFUNCTION(NetMulticast, Reliable) // Client RPC
	void MulticastJumpBuff(float JumpVelocity);// Jump속도값 변경을 알라주는 함수

};
