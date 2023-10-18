#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MultiplayerGameState.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYER_API AMultiplayerGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class AMultiplayerPlayerState* ScoringPlayer);

	UPROPERTY(Replicated) 
	TArray<AMultiplayerPlayerState*> TopScoringPlayers;

private:
	float TopScore = 0.0f;
};
