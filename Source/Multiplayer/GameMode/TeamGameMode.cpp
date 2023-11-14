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

float ATeamGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	TWeakObjectPtr<AMultiplayerPlayerState> AttackerPState = Attacker->GetPlayerState<AMultiplayerPlayerState>();
	TWeakObjectPtr<AMultiplayerPlayerState> VictimPState = Victim->GetPlayerState<AMultiplayerPlayerState>();
	if (AttackerPState == nullptr || VictimPState == nullptr) return BaseDamage;

	if (VictimPState == AttackerPState) // �ڱ� ������ ������ ������ ���
	{
		return BaseDamage;
	}
	if (AttackerPState->GetTeam() == VictimPState->GetTeam()) // ���� ���� ���
	{
		return 0.0f; // ���� ���� ���, �������� ������ �ʴ´�.
	}

	return BaseDamage; // ������ ��ȯ
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
