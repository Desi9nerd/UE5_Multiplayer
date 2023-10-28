#include "BuffComponent.h"
#include "Multiplayer/Character/BaseCharacter.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (bHealing == false || Character == nullptr || Character->IsElimmed()) return; // 예외처리

	const float HealThisFrame = HealingRate * DeltaTime; // HealThisFrame는 초당 체력이 회복되는 수치
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.0f, Character->GetMaxHealth()));
	Character->UpdateHUDHealth(); // 체력HUD 업데이트
	AmountToHeal -= HealThisFrame; // 체력 회복수치에서 현재 프레임(=여기서는 초)에 회복된 정도를 빼서 업데이트

	if (AmountToHeal <= 0.0f || Character->GetHealth() >= Character->GetMaxHealth()) // 다음과 같은 상태면 회복 종료
	{
		bHealing = false;
		AmountToHeal = 0.0f;
	}
}
