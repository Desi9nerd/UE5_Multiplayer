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

	//** Teams
	TArray<AMultiplayerPlayerState*> RedTeam;
	TArray<AMultiplayerPlayerState*> BlueTeam;
	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.0f;
	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.0f;

	UFUNCTION()
	void OnRep_RedTeamScore();
	UFUNCTION()
	void OnRep_BlueTeamScore();

private:
	float TopScore = 0.0f;
};