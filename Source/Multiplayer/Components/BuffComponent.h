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
	friend class ABaseCharacter; // BaseCharacter Ŭ������ friend Ŭ������ ����.

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Heal(float HealAmount, float HealingTime);
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed); // Speed �ʱⰪ
	void SetInitialJumpVelocity(float Velocity); // Jump�ӵ� �ʱⰪ
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime); // Speed Buff ����
	void BuffJump(float BuffJumpVelocity, float BuffTime);  // Jump Buff ����

protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);

private:
	TWeakObjectPtr<class ABaseCharacter> Character;

	//** Health Buff
	bool bHealing = false; // ȸ�� true/false
	float HealingRate = 0; // ȸ���Ǵ� �ӵ�
	float AmountToHeal = 0.0f; // ü�� ȸ����ġ

	//** Speed Buff
	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds(); // Speed�� ������� �ǵ����� �Լ�
	float InitialBaseSpeed;
	float InitialCrouchSpeed;
	UFUNCTION(NetMulticast, Reliable) // Client RPC
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed); // Speed ������ �˶��ִ� �Լ�

	//** Jump Buff
	FTimerHandle JumpBuffTimer;
	void ResetJump(); // Jump�ӵ����� ������� �ǵ����� �Լ�
	float InitialJumpVelocity;
	UFUNCTION(NetMulticast, Reliable) // Client RPC
	void MulticastJumpBuff(float JumpVelocity);// Jump�ӵ��� ������ �˶��ִ� �Լ�

};
