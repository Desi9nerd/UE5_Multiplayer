#include "BaseCharacterAnimInstance.h"
#include "BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBaseCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BaseCharacter = Cast<ABaseCharacter>(TryGetPawnOwner());
}

void UBaseCharacterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BaseCharacter == nullptr)
	{
		BaseCharacter = Cast<ABaseCharacter>(TryGetPawnOwner());
	}
	if (BaseCharacter == nullptr) return;

	FVector Velocity = BaseCharacter->GetVelocity();
	Velocity.Z = 0.0f; //Z속도를 0으로 설정
	Speed = Velocity.Size();

	bIsInAir = BaseCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BaseCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0f ? true : false;
	bWeaponEquipped = BaseCharacter->IsWeaponEquipped();
	bIsCrouched = BaseCharacter->bIsCrouched;
	bAiming = BaseCharacter->IsAiming();//bAiming 변수의 true/false값을 캐릭터의 조준 true/false값으로 설정해준다.
}
