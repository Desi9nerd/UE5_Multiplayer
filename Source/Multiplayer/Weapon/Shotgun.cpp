#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Multiplayer/Components/LagCompensationComponent.h"
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
		TMap<ABaseCharacter*, uint32> HeadShotHitMap;// ĳ���Ͱ� ������ ��弦 �ǰ�ó�� �ǵ��� HitMap������ ��´�

		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit; // �ǰ���ġ
			WeaponTraceHit(Start, HitTarget, FireHit); // �ǰ���ġ(=FireHit)�� ����ؼ� ������Ʈ

			ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(FireHit.GetActor());
			if (BaseCharacter)
			{
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("head"); // �ǰݵ� ���� "head"�� true

				if (bHeadShot) // ��弦�� ���
				{
					if (HeadShotHitMap.Contains(BaseCharacter))	HeadShotHitMap[BaseCharacter]++;
					else HeadShotHitMap.Emplace(BaseCharacter, 1);

				}
				else // ��弦�� �ƴ� ���
				{
					if (HitMap.Contains(BaseCharacter))	HitMap[BaseCharacter]++; // ù �ǰ� ������ �ǰ� �� HitMap�� �� ++�ϸ鼭 ���
					else HitMap.Emplace(BaseCharacter, 1); //ù �ǰ� �� TMap<ABaseCharacter*, uint32> HitMap�� ���
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
		TArray<ABaseCharacter*> HitCharacters;
		TMap<ABaseCharacter*, float> DamageMap; // �� ĳ���Ϳ� ������ �������� ���ؼ� DamageMap������ ��´�
		//* DamageMap�� �ٵ� ������ ��� �� ����
		for (auto HitPair : HitMap) // �ٵ�
		{
			if (HitPair.Key) // HitMap�� ���
			{
				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage); // HitPair.Value * Damage��ŭ ������ ����

				HitCharacters.AddUnique(HitPair.Key); // �ǰ� ĳ���͵� ���
			}
		}
		//* DamageMap�� ��弦 ������ ��� �� ����
		for (auto HeadShotHitPair : HeadShotHitMap) // ��弦
		{
			if (HeadShotHitPair.Key) // HeadShotHitMap�� ���
			{
				if (DamageMap.Contains(HeadShotHitPair.Key)) // �ش� ĳ���Ͱ� DamageMap�� ��ϵǾ� �ִٸ�
					DamageMap[HeadShotHitPair.Key] += HeadShotHitPair.Value * HeadShotDamage; // �������� ���Ѵ�
				else  // �ش� ĳ���Ͱ� DamageMap�� ��ϵǾ� ���� �ʴٸ� ����Ѵ�
					DamageMap.Emplace(HeadShotHitPair.Key, HeadShotHitPair.Value * HeadShotDamage);

				HitCharacters.AddUnique(HeadShotHitPair.Key); // �ǰ� ĳ���͵� ���
			}
		}

		//* DamageMap�� ������ ���� ���� ĳ������ Total Damage�� ���Ѵ�.
		for (auto DamagePair : DamageMap)
		{
			if (DamagePair.Key && InstigatorController)
			{
				bool bCauseAuthDamage = bUseServerSideRewind == false || OwnerPawn->IsLocallyControlled();

				if (HasAuthority() && bCauseAuthDamage) // Server && no SSR
				{
					// ������ ����: Damage * HitPair.Value ��ŭ ������
					UGameplayStatics::ApplyDamage(
						DamagePair.Key,		// Character that was hit
						DamagePair.Value,	// Damage calculated in the two for loops above
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}


		if (HasAuthority() == false && bUseServerSideRewind) // Client &&  SSR
		{
			BaseCharcterOwnerCharacter = BaseCharcterOwnerCharacter == nullptr ? Cast<ABaseCharacter>(OwnerPawn) : BaseCharcterOwnerCharacter;
			MainPlayerOwnerController = MainPlayerOwnerController == nullptr ? Cast<AMainPlayerController>(InstigatorController) : MainPlayerOwnerController;

			if (BaseCharcterOwnerCharacter && MainPlayerOwnerController && BaseCharcterOwnerCharacter->GetLagCompensation() && BaseCharcterOwnerCharacter->IsLocallyControlled())
			{
				BaseCharcterOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters, // �ǰݵǴ� ĳ���͵� 
					Start,	// Muzzle ���� ��ġ
					HitTargets, // �Ű������� ���� const TArray<FVector_NetQuantize>& HitTargets
					MainPlayerOwnerController->GetServerTime() - MainPlayerOwnerController->SingleTripTime
				);
			}
		}
	}//if(IsValid(MuzzleFlashSocket))
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
