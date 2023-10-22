#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget); // Super�� �θ��� HitScanWeapon�� Fire()�Լ��� �θ��� �ʰ� �θ��� �θ��� WeaponŬ������ Fire()�Լ��� ���Ѵ�.

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return; // OwnerPawn�� ���� ��� ���� ó��

	AController* InstigatorController = OwnerPawn->GetController();
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");

	if (IsValid(MuzzleFlashSocket))
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		TMap<ABaseCharacter*, uint32> HitMap;

		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(FireHit.GetActor());
			if (BaseCharacter && HasAuthority() && InstigatorController)
			{
				if (HitMap.Contains(BaseCharacter))
				{
					HitMap[BaseCharacter]++; // ù �ǰ� ������ �ǰ� �� HitMap�� �� ++�ϸ鼭 ���
				}
				else
				{
					HitMap.Emplace(BaseCharacter, 1);//ù �ǰ� �� TMap<ABaseCharacter*, uint32> HitMap�� ���
				}
			}
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
			}
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint, 0.5f,FMath::FRandRange(-0.5f, 0.5f));
			}
		} // for

		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController) // HitMap�� ��� && Server && Controller (O)
			{
				// ������ ����: Damage * HitPair.Value ��ŭ ������
				UGameplayStatics::ApplyDamage(HitPair.Key, Damage * HitPair.Value, InstigatorController, this, UDamageType::StaticClass());
			} // if
		} // for
	} // if(IsValid(MuzzleFlashSocket))
}
