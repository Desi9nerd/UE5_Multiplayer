#include "MultiplayerGameMode.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Multiplayer/PlayerState/MultiplayerPlayerState.h"

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

	if (MatchState == MatchState::WaitingToStart)
	{
		// ���� ���� �� ���ð� - ���� �ð� + ���ӷ����ʿ� �� �ð�.
		// ���� �ð��� ���� ���� ���ĺ��� ��ϵǴ� ���ӷ����ʿ� �� �ð���ŭ �����ش�.
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.0f)
		{
			StartMatch();
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
			SelectedPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void AMultiplayerGameMode::PlayerEliminated(ABaseCharacter* ElimmedCharacter, AMainPlayerController* VictimController,
                                            AMainPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;

	TWeakObjectPtr<AMultiplayerPlayerState> AttackerPlayerState = AttackerController ? Cast<AMultiplayerPlayerState>(AttackerController->PlayerState) : nullptr;
	TWeakObjectPtr<AMultiplayerPlayerState> VictimPlayerState = VictimController ? Cast<AMultiplayerPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState.IsValid() && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.0f); // ���� ���ϱ�
	}
	if (VictimPlayerState.IsValid())
	{
		VictimPlayerState->AddToDefeats(1); // �¸�Ƚ�� ���ϱ�
	}

	if (IsValid(ElimmedCharacter))
	{
		ElimmedCharacter->Elim(); // ĳ���� �Ҹ��Ű��
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
