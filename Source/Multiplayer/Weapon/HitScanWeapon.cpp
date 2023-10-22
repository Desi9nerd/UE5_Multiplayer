#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
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
		FVector End = Start + (HitTarget - Start) * 1.25f; // LineTrace 끝점. HitTraget 위치보다 좀 더 여유있게 준다.

		FHitResult FireHit;
		TObjectPtr<UWorld> World = GetWorld();

		if (IsValid(World))
		{
			// LineTrace
			World->LineTraceSingleByChannel(
				FireHit,
				Start,
				End,
				ECollisionChannel::ECC_Visibility
			);

			FVector BeamEnd = End; // BeamEnd 변수에 LineTrace의 마지막 위치를 담는다.

			if (FireHit.bBlockingHit) // LineTrace가 BlockingHit 되었다면
			{
				BeamEnd = FireHit.ImpactPoint; // BeamEnd를 업데이트
				TObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(FireHit.GetActor());
				if (IsValid(BaseCharacter) && HasAuthority() && InstigatorController)
				{
					// 데미지 전달
					UGameplayStatics::ApplyDamage(BaseCharacter, Damage, InstigatorController, this, UDamageType::StaticClass()	);
				} // if(IsValid(BaseCharacter) && HasAuthority() && InstigatorController)

				if (ImpactParticles) // 충돌 시 파티클 스폰 
				{
					UGameplayStatics::SpawnEmitterAtLocation(World, ImpactParticles, FireHit.ImpactPoint, 	FireHit.ImpactNormal.Rotation());
				} // if (ImpactParticles)
				if (HitSound) // 충돌 시 사운드 재생
				{
					UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint);
				}
			} // if(FireHit.bBlockingHit)

			if (BeamParticles)
			{
				TWeakObjectPtr<UParticleSystemComponent> Beam = UGameplayStatics::SpawnEmitterAtLocation(
					World,
					BeamParticles,
					SocketTransform
				);
				if (Beam.IsValid())
				{
					Beam->SetVectorParameter(FName("Target"), BeamEnd);
				}
			} // if(BeamParticles)
		} // if(IsValid(World))

		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFlash, SocketTransform);
		} // if(MuzzleFlash)
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, 	FireSound, GetActorLocation());
		} // if(FireSound)
	} // if(IsValid(MuzzleFlashSocket) && IsValid(InstigatorController))
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	//** LineTrace의 시작위치(=TraceStart)와 끝위치(=HitTarget)을 매개변수로 받는다. Shotgun의 산탄을 구현하기 위해 LineTrace를 일정범위 내에서 랜덤으로 end location이 변경되도록 한 뒤 리턴한다. 
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius); 
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLoc, 4.0f, 12, FColor::Orange, true);
	DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()), FColor::Cyan,true);

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()); // LineTrace의 end location을 리턴.
}
