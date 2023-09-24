#include "BaseCharacterAnimInstance.h"
#include "BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

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
	Velocity.Z = 0.0f; //Z�ӵ��� 0���� ����
	Speed = Velocity.Size();

	bIsInAir = BaseCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BaseCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0f ? true : false;
	bWeaponEquipped = BaseCharacter->IsWeaponEquipped();
	bIsCrouched = BaseCharacter->bIsCrouched;
	bAiming = BaseCharacter->IsAiming();//bAiming ������ true/false���� ĳ������ ���� true/false������ �������ش�.

	// YawOffset: Straft������ ���� Yaw Offset ����
	FRotator AimRotation = BaseCharacter->GetBaseAimRotation();//-180 ~ 180��. local���� �ƴ� world��
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BaseCharacter->GetVelocity());
	YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BaseCharacter->GetActorRotation();//ĳ������ ȸ������ ��´�.
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.0f);
	Lean = FMath::Clamp(Interp, -90.0f, 90.0f);//lean���� -90~90�� ���̿� �ֵ��� Clamp 
}
