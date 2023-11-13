#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
#include "Multiplayer/Components/LagCompensationComponent.h"
#include "DrawDebugHelpers.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	TWeakObjectPtr<APawn> OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return; // OwnerPawn이 없다면 발사하지 않고 리턴.


	TObjectPtr<AController> InstigatorController = OwnerPawn->GetController();
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");

	if (IsValid(MuzzleFlashSocket) && IsValid(InstigatorController))
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();	   // LineTrace 시작점

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		TObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(FireHit.GetActor());
		if (IsValid(BaseCharacter) && InstigatorController)
		{
			bool bCauseAuthDamage = bUseServerSideRewind == false || OwnerPawn->IsLocallyControlled();

			if (HasAuthority() && bCauseAuthDamage) // Server && no SSR이면 데미지 전달
			{
				const float DamageToCause = FireHit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage; // DamageToCause변수값을 데미지를 입히는 본이 "head"면 HeadShotDamage, 아니면 Damage로 설정

				// 데미지 전달
				UGameplayStatics::ApplyDamage(BaseCharacter, DamageToCause, InstigatorController, this, UDamageType::StaticClass());
			}
			if (HasAuthority() == false && bUseServerSideRewind) // Client && SSR이면 ServerScoreRequest()
			{
				BaseCharcterOwnerCharacter = BaseCharcterOwnerCharacter == nullptr ? Cast<ABaseCharacter>(OwnerPawn) : BaseCharcterOwnerCharacter;
				MainPlayerOwnerController = MainPlayerOwnerController == nullptr ? Cast<AMainPlayerController>(InstigatorController) : MainPlayerOwnerController;

				if (BaseCharcterOwnerCharacter && MainPlayerOwnerController && BaseCharcterOwnerCharacter->GetLagCompensation() && BaseCharcterOwnerCharacter->IsLocallyControlled())
				{
					BaseCharcterOwnerCharacter->GetLagCompensation()->ServerScoreRequest(
						BaseCharacter,
						Start,
						HitTarget,
						MainPlayerOwnerController->GetServerTime() - MainPlayerOwnerController->SingleTripTime,
						this
					); // HitTime = ServerTime - SingleTripTime 을 사용
				}
			}
		} 
		if (ImpactParticles) // 충돌 시 파티클 스폰 
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
		} // if (ImpactParticles)
		if (HitSound) // 충돌 시 사운드 재생
		{
			UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint);
		}
		if (MuzzleFlash) // Muzzle에서 이펙트 스폰
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		} 
		if (FireSound) // 발사 사운드 재생
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
		} 
	} 
}

// TraceStart와 HitTarget 변수를 매개변수로 받아 OutHit값을 구하여 사용할 수 있게 한다.
void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	TObjectPtr<UWorld> World = GetWorld();

	if (IsValid(World))
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;

		World->LineTraceSingleByChannel(OutHit, TraceStart, End, ECollisionChannel::ECC_Visibility);

		FVector BeamEnd = End;

		if (OutHit.bBlockingHit) // LineTrace가 BlockingHit 된다면
		{
			BeamEnd = OutHit.ImpactPoint; // BeamEnd 위치를 충돌위치(=ImpactPoint)로 설정.
		}
		else // LineTrace가 BlockingHit 되지 않는다면
		{
			OutHit.ImpactPoint = End; // 충돌위치(=ImpactPoint)를 End로 설정.
		}

		//DrawDebugSphere(GetWorld(), BeamEnd, 16.0f, 12, FColor::Orange, true);//디버깅용

		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(World, 	BeamParticles, TraceStart, FRotator::ZeroRotator,true);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}
