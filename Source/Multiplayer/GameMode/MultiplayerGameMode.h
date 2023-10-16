#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MultiplayerGameMode.generated.h"

namespace MatchState
{
	extern MULTIPLAYER_API const FName Cooldown; // 경기가 끝나면 승/패를 띄운다
}

/** GameMode
 *  GameMode는 Server에만 존재한다. Client 존재X
 */

UCLASS()
class MULTIPLAYER_API AMultiplayerGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMultiplayerGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class ABaseCharacter* ElimmedCharacter, class AMainPlayerController* VictimController, AMainPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.0f; // 게임 시작 전 대기시간

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.0f; // 경기 시간

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.0f; // 경기 끝난 후 대기시간

	float LevelStartingTime = 0.0f; // 게임레벨맵에 들어간 시간

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.0f;
};
