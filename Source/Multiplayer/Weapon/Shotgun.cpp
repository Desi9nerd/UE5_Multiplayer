#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector()); // Super�� �θ��� HitScanWeapon�� Fire()�Լ��� �θ��� �ʰ� �θ��� �θ��� WeaponŬ������ Fire()�Լ��� ���Ѵ�.

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return; // OwnerPawn�� ���� ��� ���� ó��

	AController* InstigatorController = OwnerPawn->GetController();
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");

	if (IsValid(MuzzleFlashSocket))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		TMap<ABaseCharacter*, uint32> HitMap; // ĳ���Ͱ� ������ �ǰ�(=Hit)ó�� �ǵ��� HitMap������ ��´�

		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit; // �ǰ���ġ
			WeaponTraceHit(Start, HitTarget, FireHit); // �ǰ���ġ(=FireHit)�� ����ؼ� ������Ʈ

			ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(FireHit.GetActor());
			if (BaseCharacter)
			{
				if (HitMap.Contains(BaseCharacter))
				{
					HitMap[BaseCharacter]++; // ù �ǰ� ������ �ǰ� �� HitMap�� �� ++�ϸ鼭 ���
				}
				else
				{
					HitMap.Emplace(BaseCharacter, 1); //ù �ǰ� �� TMap<ABaseCharacter*, uint32> HitMap�� ���
				}

				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
				}
				if (HitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint, 0.5f, FMath::FRandRange(-0.5f, 0.5f));
				}
			}
		}

		//** HitMap�� ��ϵ� ĳ���͵鿡�� ������ ������
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController) // HitMap�� ��� && Server && Controller (O)
			{
				// ������ ����: Damage * HitPair.Value ��ŭ ������
				UGameplayStatics::ApplyDamage(
					HitPair.Key, // Character that was hit
					Damage * HitPair.Value, // Multiply Damage by number of times hit
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		} 
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets) // ���� ��ź������ ���� LineTrace�� End Loc ���� �����ϴ� �Լ�
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return; // MuzzleFlash ������ ������ �� FVector ����

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation(); // LineTrace ������(=MuzzleFlash ������ġ)

	// LineTrace�� ������ġ(=TraceStart)�� ����ġ(=HitTarget) ���. 
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		// Shotgun�� ��ź�� �����ϱ� ���� LineTrace�� �������� ������ �������� end location�� ����ǵ��� �� �� �����Ѵ�.
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();

		HitTargets.Add(ToEndLoc);  // LineTrace�� end location�� HitTargets�� �߰��Ѵ�.
	}
}
