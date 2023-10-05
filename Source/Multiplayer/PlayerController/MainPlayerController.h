#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

/** PlayerController
 *  HUD를 Update 한다.
 */
UCLASS()
class MULTIPLAYER_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetHUDHealth(float Health, float MaxHealth); // HealthBar에 Health/MaxHealth

protected:
	virtual void BeginPlay() override;

private:
	TObjectPtr<class AMainHUD> MainHUD;
};
