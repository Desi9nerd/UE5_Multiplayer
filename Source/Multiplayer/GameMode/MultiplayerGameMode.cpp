#include "MultiplayerGameMode.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Multiplayer/PlayerState/MultiplayerPlayerState.h"
#include "Multiplayer/GameState/MultiplayerGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

AMultiplayerGameMode::AMultiplayerGameMode()
{
	bDelayedStart = true; // true�� GameMode�� start �Ǳ� ���� waiting ���°� �ȴ�.
}

void AMultiplayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds(); // ���ӷ����ʿ� �� �ð�
}

void AMultiplayerGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart) // ��� ���� �� ���ð�
	{
		// ���� ���� �� ���ð� - ���� �ð� + ���ӷ����ʿ� �� �ð�.
		// ���� �ð��� ���� ���� ���ĺ��� ��ϵǴ� ���ӷ����ʿ� �� �ð���ŭ �����ش�.
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.0f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress) // ��� ��
	{
		// ���� ���� �� ���ð� - ���� �ð� + ���ӷ����ʿ� �� �ð� + ������ ���ð�
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime + MatchTime;
		if (CountdownTime <= 0.0f)
		{
			SetMatchState(MatchState::Cooldown); // Cooldown ���� ���� �� ��� �� ���ð�
		}
	}
	else if (MatchState == MatchState::Cooldown) // ��� ���� �� ���ð�
	{
		// ���� ���� �� ���ð� - ���� �ð� + ���ӷ����ʿ� �� �ð� + ������ ���ð� + ������ ��� ���� �� ���ð�
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime + MatchTime + CooldownTime;
		if (CountdownTime <= 0.0f) // ������ ��� ���� �� ���ð��� ���� 0 ���ϰ� �Ǿ�����
		{
			RestartGame(); // ��� �����. GameMode ���� Ŭ������ ���ǵ� �Լ� ��.
		}
	}
}

void AMultiplayerGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet(); 

	// GameMode�� MatchState�� ����Ǹ� �������� �ش�Ǵ� PlayerController�� ã�� MatchState�� �����Ѵ�.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		TWeakObjectPtr<AMainPlayerController> SelectedPlayer = Cast<AMainPlayerController>(*It);
		if (SelectedPlayer.IsValid())
		{
			SelectedPlayer->OnMatchStateSet(MatchState, bTeamsMatch);
		}
	}
}

void AMultiplayerGameMode::PlayerEliminated(ABaseCharacter* ElimmedCharacter, AMainPlayerController* VictimController, AMainPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;

	TObjectPtr<AMultiplayerPlayerState> AttackerPlayerState = AttackerController ? Cast<AMultiplayerPlayerState>(AttackerController->PlayerState) : nullptr;
	TObjectPtr<AMultiplayerPlayerState> VictimPlayerState = VictimController ? Cast<AMultiplayerPlayerState>(VictimController->PlayerState) : nullptr;
	TObjectPtr<AMultiplayerGameState> MultiplayerGameState = GetGameState<AMultiplayerGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState
		&& MultiplayerGameState)
	{
		TArray<AMultiplayerPlayerState*> PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : MultiplayerGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.Add(LeadPlayer); // 1�� �÷��̾� ���
		}

		AttackerPlayerState->AddToScore(1.0f); // ���� ���ϱ�
		MultiplayerGameState->UpdateTopScore(AttackerPlayerState);

		if (MultiplayerGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			TObjectPtr<ABaseCharacter> Leader = Cast<ABaseCharacter>(AttackerPlayerState->GetPawn());
			if (IsValid(Leader))
			{
				Leader->MulticastGainedTheLead(); // 1�� Crown ����
			}
		}

		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			if (false == MultiplayerGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				TWeakObjectPtr<ABaseCharacter> Loser = Cast<ABaseCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser.IsValid())
				{
					Loser->MulticastLostTheLead(); // 1��� �з����� Crown ���� ���ֱ�
				}
			}
		}
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1); // �¸�Ƚ�� ���ϱ�
	}

	if (IsValid(ElimmedCharacter))
	{
		ElimmedCharacter->Elim(false); // ĳ���� �Ҹ��Ű��
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		TWeakObjectPtr<AMainPlayerController> MainPlayerController = Cast<AMainPlayerController>(*It);
		if (MainPlayerController.IsValid() && AttackerPlayerState && VictimPlayerState)
		{
			MainPlayerController->BroadcastElim(AttackerPlayerState, VictimPlayerState); // �����, �ǻ��� ����
		}
	}
}

void AMultiplayerGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}

	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);

		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]); // Player �����
	}
}

void AMultiplayerGameMode::PlayerLeftGame(AMultiplayerPlayerState* PlayerLeaving) // �÷��̾� ���� ����
{
	if (PlayerLeaving == nullptr) return;

	TWeakObjectPtr<AMultiplayerGameState> MultiplayerGameState = GetGameState<AMultiplayerGameState>();
	if (MultiplayerGameState.IsValid() && MultiplayerGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		MultiplayerGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}
	TWeakObjectPtr<ABaseCharacter> CharacterLeaving = Cast<ABaseCharacter>(PlayerLeaving->GetPawn());
	if (CharacterLeaving.IsValid())
	{
		CharacterLeaving->Elim(true);
	}
}

float AMultiplayerGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}
