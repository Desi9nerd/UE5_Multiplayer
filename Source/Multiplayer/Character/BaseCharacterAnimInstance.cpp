#include "BaseCharacterAnimInstance.h"
#include "BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Multiplayer/Weapon/Weapon.h"
#include "Multiplayer/EnumTypes/ECombatState.h"

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
	EquippedWeapon = BaseCharacter->GetEquippedWeapon(); 
	bIsCrouched = BaseCharacter->bIsCrouched;
	bAiming = BaseCharacter->IsAiming();//bAiming ������ true/false���� ĳ������ ���� true/false������ �������ش�.
	TurningInPlace = BaseCharacter->GetTurningInPlace();
	bRotateRootBone = BaseCharacter->ShouldRotateRootBone();
	bElimmed = BaseCharacter->IsElimmed();

	// YawOffset: Straft������ ���� Yaw Offset ����
	FRotator AimRotation = BaseCharacter->GetBaseAimRotation();//-180 ~ 180��. local���� �ƴ� world��
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BaseCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.0f);//�ε巯�� �������� ���� ����
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BaseCharacter->GetActorRotation();//ĳ������ ȸ������ ��´�.
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.0f);
	Lean = FMath::Clamp(Interp, -90.0f, 90.0f);//lean���� -90~90�� ���̿� �ֵ��� Clamp

	AO_Yaw = BaseCharacter->GetAO_Yaw();
	AO_Pitch = BaseCharacter->GetAO_Pitch();

	//** �޼� FABRIK ������ ���� LeftHandSocket ��ġ, ȸ�� �����ϱ�
	// ������������ && �������� �Ž����� ���� && ĳ���� �Ž����� ����
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BaseCharacter->GetMesh())
	{		
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World); // EquippedWeapon->GetWeaponMesh(): weapon.h�� FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() ȣ�� // LeftHandSocket�� ���� SkeletalMesh�� ���� �߰��� ����

		FVector OutPosition;  // LeftHandSocket�� ���� ��ġ��
		FRotator OutRotation; // LeftHandSocket�� ���� ȸ����

		//** LeftHandTransform�� "hand_r"���� ��ġ���� ��������� �̵��ϰ� ���ش�.
		BaseCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		

		if (BaseCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;

			//** �������� ȸ������ Crosshair ����� Muzzle ���� ��ġ��Ű��
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World); // ������ ������ ȸ��, ����, ��ġ���� ����ش�.
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BaseCharacter->GetHitTarget())); // ȸ���� ���ϱ�: ������ġ ����, ������ġ ���� + �浹������ ���ϴ� ���� ���
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.0f);// ������ ȸ���� ���ϱ�: ���� ������ LookAtRotation������ �����Ͽ� ����

			//** ������ ���� (���Ŀ� ���� ����)
			//FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"),ERelativeTransformSpace::RTS_World);
			//FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
			//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX *1000.0f, FColor::Blue);
			//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BaseCharacter->GetHitTarget(), FColor::Red);
		}
	}

	bUseFABRIK = BaseCharacter->GetCombatState() == ECombatState::ECS_Unoccupied; // Unoccupied ������ �� FABRIK�� ����Ѵ�.
	if (BaseCharacter->IsLocallyControlled() && BaseCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade)
	{
		bUseFABRIK = (BaseCharacter->IsLocallyReloading() == false); 
	}
	// Unoccupied ������ �� && bDisableGameplay ������ false�� AimOffset�� bTransformRightHand ���
	bUseAimOffsets = BaseCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && BaseCharacter->GetDisableGameplay() == false; 
	bTransformRightHand = BaseCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && BaseCharacter->GetDisableGameplay() == false;
}
