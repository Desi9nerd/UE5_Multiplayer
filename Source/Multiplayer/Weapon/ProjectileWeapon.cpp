#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (HasAuthority() == false) return; //Authority�� ���ٸ� �Ʒ��� �߻������ ������� �ʵ��� return ���ش�.

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));//GetSocketTransform�� �Ϸ��� SkeletalMeshSocket�� �ʿ��ϴ�. �Ʒ����� ����ϱ� ���� ���� ����.

	if (IsValid(MuzzleFlashSocket)) // Muzzle ������ �ִٸ�
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());//Spawn��ġ�� �� FTransform ����
		
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();//muzzle������ġ���� �浹Ÿ������(=Crosshair��ġ���� �� linetrace�� �浹����)���� ���ϴ� ����
		FRotator TargetRotation = ToTarget.Rotation();

		if (IsValid(ProjectileClass) && InstigatorPawn) //�߻�ü Ŭ������ InstigatorPawn�� �ִٸ�
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			UWorld* World = GetWorld();

			if (IsValid(World))
			{
				// ���忡 �߻�ü(ProjectileClass)�� SocketTransform��ġ�� Spawn ��Ų��
				World->SpawnActor<AProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
					);
			}
		}
	}
}
