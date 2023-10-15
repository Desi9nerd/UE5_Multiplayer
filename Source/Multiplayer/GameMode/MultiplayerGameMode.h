#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MultiplayerGameMode.generated.h"

/** GameMode
 *  GameMode�� Server���� �����Ѵ�. Client ����X
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
	float WarmupTime = 10.0f; // ���� ���� �� ���ð�

	float LevelStartingTime = 0.0f; // ���ӷ����ʿ� �� �ð�

protected:
	virtual void BeginPlay() override;

private:
	float CountdownTime = 0.0f;
};
