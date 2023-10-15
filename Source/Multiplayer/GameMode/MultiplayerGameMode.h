#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MultiplayerGameMode.generated.h"

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

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.0f; // 게임 시작 전 대기시간

	float LevelStartingTime = 0.0f; // 게임레벨맵에 들어간 시간

protected:
	virtual void BeginPlay() override;

private:
	float CountdownTime = 0.0f;
};
