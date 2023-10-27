#include "AmmoPickup.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/Components/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter.IsValid())
	{
		TWeakObjectPtr<UCombatComponent> Combat = BaseCharacter->GetCombat();
		if (Combat.IsValid())
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount); // 줍는 무기종류와 Ammo개수를 넘긴다.
		}
	}

	Destroy(); // 소멸
}
