#include "MultiplayerGameState.h"
#include "Net/UnrealNetwork.h"
#include "Multiplayer/PlayerState/MultiplayerPlayerState.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"

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

void AMultiplayerGameState::RedTeamScores() // Red Team µæÁ¡
{
	++RedTeamScore;

	TWeakObjectPtr<AMainPlayerController> MPlayer = Cast<AMainPlayerController>(GetWorld()->GetFirstPlayerController());
	if (MPlayer.IsValid())
	{
		MPlayer->SetHUDBlueTeamScore(RedTeamScore);
	}
}

void AMultiplayerGameState::BlueTeamScores() // Blue Team µæÁ¡
{
	++BlueTeamScore;

	TWeakObjectPtr<AMainPlayerController> MPlayer = Cast<AMainPlayerController>(GetWorld()->GetFirstPlayerController());
	if (MPlayer.IsValid())
	{
		MPlayer->SetHUDRedTeamScore(BlueTeamScore);
	}
}

void AMultiplayerGameState::OnRep_RedTeamScore() // Red Team µæÁ¡
{
	TWeakObjectPtr<AMainPlayerController> MPlayer = Cast<AMainPlayerController>(GetWorld()->GetFirstPlayerController());
	if (MPlayer.IsValid())
	{
		MPlayer->SetHUDBlueTeamScore(RedTeamScore);
	}
}

void AMultiplayerGameState::OnRep_BlueTeamScore() // Blue Team µæÁ¡
{
	TWeakObjectPtr<AMainPlayerController> MPlayer = Cast<AMainPlayerController>(GetWorld()->GetFirstPlayerController());
	if (MPlayer.IsValid())
	{
		MPlayer->SetHUDRedTeamScore(BlueTeamScore);
	}
}