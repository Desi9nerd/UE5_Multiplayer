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

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		TObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(FireHit.GetActor());
		if (IsValid(BaseCharacter) && HasAuthority() && InstigatorController)
		{
			// ������ ����
			UGameplayStatics::ApplyDamage(BaseCharacter, Damage, InstigatorController, this, UDamageType::StaticClass());
		} 
		if (ImpactParticles) // �浹 �� ��ƼŬ ���� 
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
		} // if (ImpactParticles)
		if (HitSound) // �浹 �� ���� ���
		{
			UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint);
		}
		if (MuzzleFlash) // Muzzle���� ����Ʈ ����
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		} 
		if (FireSound) // �߻� ���� ���
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

		if (OutHit.bBlockingHit) // LineTrace�� BlockingHit �ȴٸ�
		{
			BeamEnd = OutHit.ImpactPoint; // �浹��ġ(=ImpactPoint)�� BeamEnd ��ġ�� ����.
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
	//** LineTrace�� ������ġ(=TraceStart)�� ����ġ(=HitTarget)�� �Ű������� �޴´�. Shotgun�� ��ź�� �����ϱ� ���� LineTrace�� �������� ������ �������� end location�� ����ǵ��� �� �� �����Ѵ�. 
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius); 
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	//** LineTrace ������
	//DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	//DrawDebugSphere(GetWorld(), EndLoc, 4.0f, 12, FColor::Orange, true);
	//DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()), FColor::Cyan,true);

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()); // LineTrace�� end location�� ����.
}

