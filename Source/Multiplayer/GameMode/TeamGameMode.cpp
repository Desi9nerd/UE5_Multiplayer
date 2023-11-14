#include "TeamGameMode.h"
#include "Multiplayer/GameState/MultiplayerGameState.h"
#include "Multiplayer/PlayerState/MultiplayerPlayerState.h"
#include "Kismet/GameplayStatics.h"

void ATeamGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	//** TEAM �����ϱ�
	TWeakObjectPtr<AMultiplayerGameState> MGameState = Cast<AMultiplayerGameState>(UGameplayStatics::GetGameState(this));
	if (MGameState.IsValid())
	{
		TWeakObjectPtr<AMultiplayerPlayerState> MPlayerState = NewPlayer->GetPlayerState<AMultiplayerPlayerState>();
		if (MPlayerState.IsValid() && MPlayerState->GetTeam() == ETeam::ET_NoTeam)
		{
			if (MGameState->BlueTeam.Num() >= MGameState->RedTeam.Num())
			{
				MGameState->RedTeam.AddUnique(MPlayerState.Get());
				MPlayerState->SetTeam(ETeam::ET_RedTeam);
			}
			else // BlueTeam �� < RedTeam ��
			{
				MGameState->BlueTeam.AddUnique(MPlayerState.Get());
				MPlayerState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}

void ATeamGameMode::Logout(AController* Exiting)
{
	//** Logout �� �ش� Team�� TArray���� �����ϱ�
	TWeakObjectPtr<AMultiplayerGameState> MGameState = Cast<AMultiplayerGameState>(UGameplayStatics::GetGameState(this));
	TWeakObjectPtr<AMultiplayerPlayerState> MPlayerState = Exiting->GetPlayerState<AMultiplayerPlayerState>();
	if (MGameState.IsValid() && MPlayerState.IsValid())
	{
		if (MGameState->RedTeam.Contains(MPlayerState))
		{
			MGameState->RedTeam.Remove(MPlayerState.Get());
		}
		if (MGameState->BlueTeam.Contains(MPlayerState))
		{
			MGameState->BlueTeam.Remove(MPlayerState.Get());
		}
	}
}

void ATeamGameMode::HandleMatchHasStarted() // Team ����
{
	Super::HandleMatchHasStarted();

	//** TEAM �����ϱ�
	TWeakObjectPtr<AMultiplayerGameState> MGameState = Cast<AMultiplayerGameState>(UGameplayStatics::GetGameState(this));
	if (MGameState.IsValid())
	{
		// MGameState->PlayerArray�� MGameState�� ������ �ִ� PlayerState�� TArray�迭
		for (auto PState : MGameState->PlayerArray)
		{
			TWeakObjectPtr<AMultiplayerPlayerState> MPlayerState = Cast<AMultiplayerPlayerState>(PState.Get());
			if (MPlayerState.IsValid() && MPlayerState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (MGameState->BlueTeam.Num() >= MGameState->RedTeam.Num())
				{
					MGameState->RedTeam.AddUnique(MPlayerState.Get()); 
					MPlayerState->SetTeam(ETeam::ET_RedTeam); 
				}
				else // BlueTeam �� < RedTeam ��
				{
					MGameState->BlueTeam.AddUnique(MPlayerState.Get()); // BlueTeam�� PlayerState�迭�� �߰�
					MPlayerState->SetTeam(ETeam::ET_BlueTeam); // BlueTeam���� �� ����
				}
			}
		}
	}
}
