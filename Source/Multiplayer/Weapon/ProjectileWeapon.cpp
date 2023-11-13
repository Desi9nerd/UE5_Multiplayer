#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
	
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));//GetSocketTransform을 하려면 SkeletalMeshSocket가 필요하다. 아래에서 사용하기 위해 변수 생성.

	TWeakObjectPtr<UWorld> World = GetWorld();
	if (IsValid(MuzzleFlashSocket) && World.IsValid()) // Muzzle 소켓이 있다면
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());//Spawn위치로 쓸 FTransform 변수
		FVector ToTarget = HitTarget - SocketTransform.GetLocation(); // muzzle소켓위치에서 충돌타겟지점(=Crosshair위치에서 쏜 linetrace의 충돌지점)으로 향하는 벡터
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = InstigatorPawn;

		TObjectPtr<AProjectile> SpawnedProjectile = nullptr;
		if (bUseServerSideRewind) // Server-side Rewind 사용O 무기
		{
			if (InstigatorPawn->HasAuthority()) // Server
			{
				if (InstigatorPawn->IsLocallyControlled()) // Server, Host - use replicated projectile
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->Damage = Damage;
					SpawnedProjectile->HeadShotDamage = HeadShotDamage;
				}
				else // Server, not locally controlled - spawn Non-Replicated projectile, SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
				}
			}
			else // Client, using SSR
			{
				if (InstigatorPawn->IsLocallyControlled()) // Client, locally controlled - spawn Non-Replicated projectile, use SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
				}
				else // Client, not locally controlled - spawn non-replicated projectile, no SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}//if(bUseServerSideRewind)

		else // Server-side Rewind 사용X 무기
		{
			if (InstigatorPawn->HasAuthority()) // Server
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage;
				SpawnedProjectile->HeadShotDamage = HeadShotDamage;
			}
		}//else
	}
}
