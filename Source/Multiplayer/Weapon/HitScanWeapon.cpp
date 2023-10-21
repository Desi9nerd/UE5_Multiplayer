#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"

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
	} // if(IsValid(MuzzleFlashSocket) && IsValid(InstigatorController))
}
