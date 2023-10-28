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
	if (bHealing == false || Character == nullptr || Character->IsElimmed()) return; // ����ó��

	const float HealThisFrame = HealingRate * DeltaTime; // HealThisFrame�� �ʴ� ü���� ȸ���Ǵ� ��ġ
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.0f, Character->GetMaxHealth()));
	Character->UpdateHUDHealth(); // ü��HUD ������Ʈ
	AmountToHeal -= HealThisFrame; // ü�� ȸ����ġ���� ���� ������(=���⼭�� ��)�� ȸ���� ������ ���� ������Ʈ

	if (AmountToHeal <= 0.0f || Character->GetHealth() >= Character->GetMaxHealth()) // ������ ���� ���¸� ȸ�� ����
	{
		bHealing = false;
		AmountToHeal = 0.0f;
	}
}
