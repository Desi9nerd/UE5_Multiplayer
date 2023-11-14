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
	bDelayedStart = true; // true면 GameMode가 start 되기 전에 waiting 상태가 된다.
}

void AMultiplayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds(); // 게임레벨맵에 들어간 시간
}

void AMultiplayerGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart) // 경기 시작 전 대기시간
	{
		// 게임 시작 전 대기시간 - 현재 시간 + 게임레벨맵에 들어간 시간.
		// 현재 시간은 게임 시작 직후부터 기록되니 게임레벨맵에 들어간 시간만큼 더해준다.
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.0f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress) // 경기 중
	{
		// 게임 시작 전 대기시간 - 현재 시간 + 게임레벨맵에 들어간 시간 + 설정한 경기시간
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime + MatchTime;
		if (CountdownTime <= 0.0f)
		{
			SetMatchState(MatchState::Cooldown); // Cooldown 상태 변경 후 경기 후 대기시간
		}
	}
	else if (MatchState == MatchState::Cooldown) // 경기 끝난 후 대기시간
	{
		// 게임 시작 전 대기시간 - 현재 시간 + 게임레벨맵에 들어간 시간 + 설정한 경기시간 + 설정한 경기 끝난 후 대기시간
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime + MatchTime + CooldownTime;
		if (CountdownTime <= 0.0f) // 설정한 경기 끝난 후 대기시간이 지나 0 이하가 되었을때
		{
			RestartGame(); // 경기 재시작. GameMode 내장 클래스에 정의된 함수 콜.
		}
	}
}

void AMultiplayerGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet(); 

	// GameMode의 MatchState이 변경되면 서버에서 해당되는 PlayerController를 찾아 MatchState을 설정한다.
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
			PlayersCurrentlyInTheLead.Add(LeadPlayer); // 1등 플레이어 기록
		}

		AttackerPlayerState->AddToScore(1.0f); // 점수 더하기
		MultiplayerGameState->UpdateTopScore(AttackerPlayerState);

		if (MultiplayerGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			TObjectPtr<ABaseCharacter> Leader = Cast<ABaseCharacter>(AttackerPlayerState->GetPawn());
			if (IsValid(Leader))
			{
				Leader->MulticastGainedTheLead(); // 1등 Crown 띄우기
			}
		}

		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			if (false == MultiplayerGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				TWeakObjectPtr<ABaseCharacter> Loser = Cast<ABaseCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser.IsValid())
				{
					Loser->MulticastLostTheLead(); // 1등에서 밀려나면 Crown 띄운거 없애기
				}
			}
		}
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1); // 승리횟수 더하기
	}

	if (IsValid(ElimmedCharacter))
	{
		ElimmedCharacter->Elim(false); // 캐릭터 소멸시키기
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		TWeakObjectPtr<AMainPlayerController> MainPlayerController = Cast<AMainPlayerController>(*It);
		if (MainPlayerController.IsValid() && AttackerPlayerState && VictimPlayerState)
		{
			MainPlayerController->BroadcastElim(AttackerPlayerState, VictimPlayerState); // 사살자, 피살자 띄우기
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

		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]); // Player 재시작
	}
}

void AMultiplayerGameMode::PlayerLeftGame(AMultiplayerPlayerState* PlayerLeaving) // 플레이어 게임 퇴장
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
