#include "ShieldPickup.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/Components/BuffComponent.h"

void AShieldPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter.IsValid())
	{
		TWeakObjectPtr<UBuffComponent> Buff = BaseCharacter->GetBuff();
		if (Buff.IsValid())
		{
			Buff->ReplenishShield(ShieldReplenishAmount, ShieldReplenishTime);
		}
	}

	Destroy();
}
