#include "TeamGameMode.h"
#include "Multiplayer/GameState/MultiplayerGameState.h"
#include "Multiplayer/PlayerState/MultiplayerPlayerState.h"
#include "Kismet/GameplayStatics.h"

void ATeamGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	//** TEAM 배정하기
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
			else // BlueTeam 수 < RedTeam 수
			{
				MGameState->BlueTeam.AddUnique(MPlayerState.Get());
				MPlayerState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}

void ATeamGameMode::Logout(AController* Exiting)
{
	//** Logout 시 해당 Team의 TArray에서 제거하기
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

	if (VictimPState == AttackerPState) // 자기 스스로 데미지 입히는 경우
	{
		return BaseDamage;
	}
	if (AttackerPState->GetTeam() == VictimPState->GetTeam()) // 같은 팀인 경우
	{
		return 0.0f; // 같은 팀인 경우, 데미지를 입히지 않는다.
	}

	return BaseDamage; // 데미지 반환
}

void ATeamGameMode::HandleMatchHasStarted() // Team 배정
{
	Super::HandleMatchHasStarted();

	//** TEAM 배정하기
	TWeakObjectPtr<AMultiplayerGameState> MGameState = Cast<AMultiplayerGameState>(UGameplayStatics::GetGameState(this));
	if (MGameState.IsValid())
	{
		// MGameState->PlayerArray는 MGameState이 가지고 있는 PlayerState들 TArray배열
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
				else // BlueTeam 수 < RedTeam 수
				{
					MGameState->BlueTeam.AddUnique(MPlayerState.Get()); // BlueTeam의 PlayerState배열에 추가
					MPlayerState->SetTeam(ETeam::ET_BlueTeam); // BlueTeam으로 팀 설정
				}
			}
		}
	}
}
