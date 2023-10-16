#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MultiplayerGameMode.generated.h"

namespace MatchState
{
	extern MULTIPLAYER_API const FName Cooldown; // ��Ⱑ ������ ��/�и� ����
}

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

	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.0f; // ���� ���� �� ���ð�

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.0f; // ��� �ð�

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.0f; // ��� ���� �� ���ð�

	float LevelStartingTime = 0.0f; // ���ӷ����ʿ� �� �ð�

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.0f;
};
