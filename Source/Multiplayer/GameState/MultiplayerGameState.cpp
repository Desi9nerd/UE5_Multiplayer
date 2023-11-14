#include "MultiplayerGameState.h"
#include "Net/UnrealNetwork.h"
#include "Multiplayer/PlayerState/MultiplayerPlayerState.h"

void AMultiplayerGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMultiplayerGameState, TopScoringPlayers);
	DOREPLIFETIME(AMultiplayerGameState, RedTeamScore);
	DOREPLIFETIME(AMultiplayerGameState, BlueTeamScore);
}

void AMultiplayerGameState::UpdateTopScore(AMultiplayerPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0) 
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScore) 
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void AMultiplayerGameState::RedTeamScores() // Red Team µÊ¡°
{
	++RedTeamScore;
}

void AMultiplayerGameState::BlueTeamScores() // Blue Team µÊ¡°
{
	++BlueTeamScore;
}

void AMultiplayerGameState::OnRep_RedTeamScore()
{

}

void AMultiplayerGameState::OnRep_BlueTeamScore()
{

}