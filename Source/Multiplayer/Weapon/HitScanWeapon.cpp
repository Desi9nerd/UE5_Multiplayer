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

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		TObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(FireHit.GetActor());
		if (IsValid(BaseCharacter) && HasAuthority() && InstigatorController)
		{
			// 데미지 전달
			UGameplayStatics::ApplyDamage(BaseCharacter, Damage, InstigatorController, this, UDamageType::StaticClass());
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

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	TObjectPtr<UWorld> World = GetWorld();

	if (IsValid(World))
	{
		FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;

		World->LineTraceSingleByChannel(OutHit, TraceStart, End, ECollisionChannel::ECC_Visibility);

		FVector BeamEnd = End;

		if (OutHit.bBlockingHit) // LineTrace가 BlockingHit 된다면
		{
			BeamEnd = OutHit.ImpactPoint; // 충돌위치(=ImpactPoint)를 BeamEnd 위치로 설정.
		}
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

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	//** LineTrace의 시작위치(=TraceStart)와 끝위치(=HitTarget)을 매개변수로 받는다. Shotgun의 산탄을 구현하기 위해 LineTrace를 일정범위 내에서 랜덤으로 end location이 변경되도록 한 뒤 리턴한다. 
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius); 
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	//** LineTrace 디버깅용
	//DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	//DrawDebugSphere(GetWorld(), EndLoc, 4.0f, 12, FColor::Orange, true);
	//DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()), FColor::Cyan,true);

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()); // LineTrace의 end location을 리턴.
}

