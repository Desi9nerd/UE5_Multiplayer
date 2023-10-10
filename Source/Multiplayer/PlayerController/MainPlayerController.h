#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

/** PlayerController
 *  HUD�� Update �Ѵ�.
 */
UCLASS()
class MULTIPLAYER_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetHUDHealth(float Health, float MaxHealth); // HealthBar�� Health/MaxHealth
	void SetHUDScore(float Score); // ���� �ű��
	void SetHUDDefeats(int32 Defeats); // �¸�Ƚ�� �ű��
	void SetHUDWeaponAmmo(int32 Ammo); // �Ѿ� �� ������Ʈ�ؼ� ����
	virtual void OnPossess(APawn* InPawn) override; // possed�� Pawn�� �����ϴ� �Լ�

protected:
	virtual void BeginPlay() override;

private:
	TObjectPtr<class AMainHUD> MainHUD;
};
