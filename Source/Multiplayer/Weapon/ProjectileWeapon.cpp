#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (HasAuthority() == false) return; //Authority가 없다면 아래의 발사과정이 실행되지 않도록 return 해준다.

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));//GetSocketTransform을 하려면 SkeletalMeshSocket가 필요하다. 아래에서 사용하기 위해 변수 생성.

	if (IsValid(MuzzleFlashSocket)) // Muzzle 소켓이 있다면
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());//Spawn위치로 쓸 FTransform 변수
		
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();//muzzle소켓위치에서 충돌타겟지점(=Crosshair위치에서 쏜 linetrace의 충돌지점)으로 향하는 벡터
		FRotator TargetRotation = ToTarget.Rotation();

		if (IsValid(ProjectileClass) && InstigatorPawn) //발사체 클래스랑 InstigatorPawn가 있다면
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			UWorld* World = GetWorld();

			if (IsValid(World))
			{
				// 월드에 발사체(ProjectileClass)를 SocketTransform위치에 Spawn 시킨다
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
