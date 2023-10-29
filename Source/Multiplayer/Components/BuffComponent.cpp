#include "BuffComponent.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	ShieldRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
	bReplenishingShield = true;
	ShieldReplenishRate = ShieldAmount / ReplenishTime;
	ShieldReplenishAmount += ShieldAmount;
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

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
	if (bReplenishingShield == false || Character == nullptr || Character->IsElimmed()) return;

	const float ReplenishThisFrame = ShieldReplenishRate * DeltaTime; // ReplenishThisFrame�� �ʴ� Shield�� ȸ���Ǵ� ��ġ
	Character->SetShield(FMath::Clamp(Character->GetShield() + ReplenishThisFrame, 0.0f, Character->GetMaxShield()));
	Character->UpdateHUDShield(); // �ǵ�HUD ������Ʈ
	ShieldReplenishAmount -= ReplenishThisFrame; // Shield ȸ����ġ���� ���� ������(=���⼭�� ��)�� ȸ���� ������ ���� ������Ʈ

	if (ShieldReplenishAmount <= 0.0f || Character->GetShield() >= Character->GetMaxShield())
	{
		bReplenishingShield = false;
		ShieldReplenishAmount = 0.0f;
	}
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
	if (Character == nullptr) return;

	// Speed Buff ���ӽð� ����.
	Character->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &UBuffComponent::ResetSpeeds, BuffTime);

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed; // �ȱ� �ִ�ӵ� BuffBaseSpeed�� ����
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
	}
	MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed); // Client�� �˷���
}

void UBuffComponent::ResetSpeeds() // Speed�� ������� �ǵ����� �Լ�
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr) return;

	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed); // Client�� �˷���
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed) // Client RPC
{
	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void UBuffComponent::SetInitialJumpVelocity(float Velocity)
{
	InitialJumpVelocity = Velocity;
}

void UBuffComponent::BuffJump(float BuffJumpVelocity, float BuffTime)
{
	if (Character == nullptr) return;

	// Jump Buff ���ӽð� ����. 
	Character->GetWorldTimerManager().SetTimer(JumpBuffTimer, this, &UBuffComponent::ResetJump, BuffTime);

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = BuffJumpVelocity;
	}
	MulticastJumpBuff(BuffJumpVelocity); // Client�� �˷���
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpVelocity) // Client RPC
{
	if (Character.IsValid() && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
	}
}

void UBuffComponent::ResetJump()
{
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
	}
	MulticastJumpBuff(InitialJumpVelocity); // Client�� �˷���
}
