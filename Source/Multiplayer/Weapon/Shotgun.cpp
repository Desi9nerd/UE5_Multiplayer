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
	AWeapon::Fire(FVector()); // Super로 부모인 HitScanWeapon의 Fire()함수를 부르지 않고 부모의 부모인 Weapon클래스의 Fire()함수를 콜한다.

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return; // OwnerPawn이 없는 경우 예외 처리

	AController* InstigatorController = OwnerPawn->GetController();
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");

	if (IsValid(MuzzleFlashSocket))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		TMap<ABaseCharacter*, uint32> HitMap; // 캐릭터가 여러번 피격(=Hit)처리 되도록 HitMap변수에 담는다
		TMap<ABaseCharacter*, uint32> HeadShotHitMap;// 캐릭터가 여러번 헤드샷 피격처리 되도록 HitMap변수에 담는다

		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit; // 피격위치
			WeaponTraceHit(Start, HitTarget, FireHit); // 피격위치(=FireHit)를 계산해서 업데이트

			ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(FireHit.GetActor());
			if (BaseCharacter)
			{
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("head"); // 피격된 본이 "head"면 true

				if (bHeadShot) // 헤드샷인 경우
				{
					if (HeadShotHitMap.Contains(BaseCharacter))	HeadShotHitMap[BaseCharacter]++;
					else HeadShotHitMap.Emplace(BaseCharacter, 1);

				}
				else // 헤드샷이 아닌 경우
				{
					if (HitMap.Contains(BaseCharacter))	HitMap[BaseCharacter]++; // 첫 피격 이후의 피격 시 HitMap에 값 ++하면서 기록
					else HitMap.Emplace(BaseCharacter, 1); //첫 피격 시 TMap<ABaseCharacter*, uint32> HitMap에 등록
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

		//** HitMap에 등록된 캐릭터들에게 데미지 입히기
		TArray<ABaseCharacter*> HitCharacters;
		TMap<ABaseCharacter*, float> DamageMap; // 각 캐릭터에 가해질 데미지를 더해서 DamageMap변수에 담는다
		//* DamageMap에 바디샷 데미지 계산 후 저장
		for (auto HitPair : HitMap) // 바디샷
		{
			if (HitPair.Key) // HitMap에 등록
			{
				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage); // HitPair.Value * Damage만큼 데미지 저장

				HitCharacters.AddUnique(HitPair.Key); // 피격 캐릭터들 등록
			}
		}
		//* DamageMap에 헤드샷 데미지 계산 후 저장
		for (auto HeadShotHitPair : HeadShotHitMap) // 헤드샷
		{
			if (HeadShotHitPair.Key) // HeadShotHitMap에 등록
			{
				if (DamageMap.Contains(HeadShotHitPair.Key)) // 해당 캐릭터가 DamageMap에 등록되어 있다면
					DamageMap[HeadShotHitPair.Key] += HeadShotHitPair.Value * HeadShotDamage; // 데미지를 더한다
				else  // 해당 캐릭터가 DamageMap에 등록되어 있지 않다면 등록한다
					DamageMap.Emplace(HeadShotHitPair.Key, HeadShotHitPair.Value * HeadShotDamage);

				HitCharacters.AddUnique(HeadShotHitPair.Key); // 피격 캐릭터들 등록
			}
		}

		//* DamageMap을 루프를 돌아 각각 캐릭터의 Total Damage를 구한다.
		for (auto DamagePair : DamageMap)
		{
			if (DamagePair.Key && InstigatorController)
			{
				bool bCauseAuthDamage = bUseServerSideRewind == false || OwnerPawn->IsLocallyControlled();

				if (HasAuthority() && bCauseAuthDamage) // Server && no SSR
				{
					// 데미지 전달: Damage * HitPair.Value 만큼 데미지
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
					HitCharacters, // 피격되는 캐릭터들 
					Start,	// Muzzle 소켓 위치
					HitTargets, // 매개변수로 들어온 const TArray<FVector_NetQuantize>& HitTargets
					MainPlayerOwnerController->GetServerTime() - MainPlayerOwnerController->SingleTripTime
				);
			}
		}
	}//if(IsValid(MuzzleFlashSocket))
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets) // 샷건 산탄분포를 위해 LineTrace의 End Loc 랜덤 변경하는 함수
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return; // MuzzleFlash 소켓이 없으면 빈 FVector 리턴

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation(); // LineTrace 시작점(=MuzzleFlash 소켓위치)

	// LineTrace의 시작위치(=TraceStart)와 끝위치(=HitTarget) 사용. 
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		// Shotgun의 산탄을 구현하기 위해 LineTrace를 일정범위 내에서 랜덤으로 end location이 변경되도록 한 뒤 리턴한다.
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();

		HitTargets.Add(ToEndLoc);  // LineTrace의 end location을 HitTargets에 추가한다.
	}
}
