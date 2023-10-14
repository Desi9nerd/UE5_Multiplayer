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
	virtual void Tick(float DeltaTime) override;
	void SetHUDHealth(float Health, float MaxHealth); // HealthBar에 Health/MaxHealth
	void SetHUDScore(float Score); // 점수 매기기
	void SetHUDDefeats(int32 Defeats); // 승리횟수 매기기
	void SetHUDWeaponAmmo(int32 Ammo); // 총알 수 업데이트해서 띄우기
	void SetHUDCarriedAmmo(int32 Ammo);// 최대 총알 수 띄우기
	void SetHUDMatchCountdown(float CountdownTime); // 남은시간 띄우기
	virtual void OnPossess(APawn* InPawn) override; // possed된 Pawn에 접근하는 함수

protected:
	virtual void BeginPlay() override;
	void SetHUDTime();

private:
	TObjectPtr<class AMainHUD> MainHUD;

	float MatchTime = 120.0f;
	uint32 CountdownInt = 0;
};
