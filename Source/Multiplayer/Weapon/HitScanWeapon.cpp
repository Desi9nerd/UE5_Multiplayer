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
	if (OwnerPawn == nullptr) return; // OwnerPawn�� ���ٸ� �߻����� �ʰ� ����.


	TObjectPtr<AController> InstigatorController = OwnerPawn->GetController();
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");

	if (IsValid(MuzzleFlashSocket) && IsValid(InstigatorController))
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();	   // LineTrace ������
		FVector End = Start + (HitTarget - Start) * 1.25f; // LineTrace ����. HitTraget ��ġ���� �� �� �����ְ� �ش�.

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

			FVector BeamEnd = End; // BeamEnd ������ LineTrace�� ������ ��ġ�� ��´�.

			if (FireHit.bBlockingHit) // LineTrace�� BlockingHit �Ǿ��ٸ�
			{
				BeamEnd = FireHit.ImpactPoint; // BeamEnd�� ������Ʈ
				TObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(FireHit.GetActor());
				if (IsValid(BaseCharacter) && HasAuthority() && InstigatorController)
				{
					// ������ ����
					UGameplayStatics::ApplyDamage(BaseCharacter, Damage, InstigatorController, this, UDamageType::StaticClass()	);
				} // if(IsValid(BaseCharacter) && HasAuthority() && InstigatorController)

				if (ImpactParticles) // �浹 �� ��ƼŬ ���� 
				{
					UGameplayStatics::SpawnEmitterAtLocation(World, ImpactParticles, FireHit.ImpactPoint, 	FireHit.ImpactNormal.Rotation());
				} // if (ImpactParticles)
				if (HitSound) // �浹 �� ���� ���
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
	//** LineTrace�� ������ġ(=TraceStart)�� ����ġ(=HitTarget)�� �Ű������� �޴´�. Shotgun�� ��ź�� �����ϱ� ���� LineTrace�� �������� ������ �������� end location�� ����ǵ��� �� �� �����Ѵ�. 
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius); 
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLoc, 4.0f, 12, FColor::Orange, true);
	DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()), FColor::Cyan,true);

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()); // LineTrace�� end location�� ����.
}
