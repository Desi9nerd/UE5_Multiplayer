#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Flag.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYER_API AFlag : public AWeapon
{
	GENERATED_BODY()

public:
	AFlag();

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* FlagMesh;
};
