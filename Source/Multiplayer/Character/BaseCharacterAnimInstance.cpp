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
	Velocity.Z = 0.0f; //Z속도를 0으로 설정
	Speed = Velocity.Size();

	bIsInAir = BaseCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BaseCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0f ? true : false;
	bWeaponEquipped = BaseCharacter->IsWeaponEquipped();
	EquippedWeapon = BaseCharacter->GetEquippedWeapon(); 
	bIsCrouched = BaseCharacter->bIsCrouched;
	bAiming = BaseCharacter->IsAiming();//bAiming 변수의 true/false값을 캐릭터의 조준 true/false값으로 설정해준다.
	TurningInPlace = BaseCharacter->GetTurningInPlace();
	bRotateRootBone = BaseCharacter->ShouldRotateRootBone();
	bElimmed = BaseCharacter->IsElimmed();

	// YawOffset: Straft동작을 위한 Yaw Offset 설정
	FRotator AimRotation = BaseCharacter->GetBaseAimRotation();//-180 ~ 180도. local값이 아닌 world값
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BaseCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.0f);//부드러운 움직임을 위해 보간
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BaseCharacter->GetActorRotation();//캐릭터의 회전값을 담는다.
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.0f);
	Lean = FMath::Clamp(Interp, -90.0f, 90.0f);//lean값이 -90~90도 사이에 있도록 Clamp

	AO_Yaw = BaseCharacter->GetAO_Yaw();
	AO_Pitch = BaseCharacter->GetAO_Pitch();

	//** 왼손 FABRIK 적용을 위한 LeftHandSocket 위치, 회전 조정하기
	// 무기장착여부 && 장착무기 매쉬정보 존재 && 캐릭터 매쉬정보 존재
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BaseCharacter->GetMesh())
	{		
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World); // EquippedWeapon->GetWeaponMesh(): weapon.h의 FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() 호출 // LeftHandSocket은 무기 SkeletalMesh에 내가 추가한 소켓

		FVector OutPosition;  // LeftHandSocket에 사용될 위치값
		FRotator OutRotation; // LeftHandSocket에 사용될 회전값

		//** LeftHandTransform이 "hand_r"소켓 위치에서 상대적으로 이동하게 해준다.
		BaseCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		

		if (BaseCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;

			//** 오른손을 회전시켜 Crosshair 방향과 Muzzle 방향 일치시키기
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World); // 오른손 소켓의 회전, 비율, 위치값을 담아준다.
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BaseCharacter->GetHitTarget())); // 회전값 구하기: 시작위치 벡터, 시작위치 벡터 + 충돌지점을 향하는 벡터 사용
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.0f);// 오른손 회전값 구하기: 현재 값에서 LookAtRotation값으로 보간하여 변경

			//** 디버깅용 라인 (추후에 삭제 예정)
			//FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"),ERelativeTransformSpace::RTS_World);
			//FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
			//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX *1000.0f, FColor::Blue);
			//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BaseCharacter->GetHitTarget(), FColor::Red);
		}
	}

	bUseFABRIK = BaseCharacter->GetCombatState() == ECombatState::ECS_Unoccupied; // Unoccupied 상태일 때 FABRIK를 사용한다.
	if (BaseCharacter->IsLocallyControlled() && BaseCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade)
	{
		bUseFABRIK = (BaseCharacter->IsLocallyReloading() == false); 
	}
	// Unoccupied 상태일 때 && bDisableGameplay 변수가 false면 AimOffset와 bTransformRightHand 사용
	bUseAimOffsets = BaseCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && BaseCharacter->GetDisableGameplay() == false; 
	bTransformRightHand = BaseCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && BaseCharacter->GetDisableGameplay() == false;
}
