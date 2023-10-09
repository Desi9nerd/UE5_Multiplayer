#include "MultiplayerGameMode.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Multiplayer/PlayerState/MultiplayerPlayerState.h"

void AMultiplayerGameMode::PlayerEliminated(ABaseCharacter* ElimmedCharacter, AMainPlayerController* VictimController,
	AMainPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;

	TWeakObjectPtr<AMultiplayerPlayerState> AttackerPlayerState = AttackerController ? Cast<AMultiplayerPlayerState>(AttackerController->PlayerState) : nullptr;
	TWeakObjectPtr<AMultiplayerPlayerState> VictimPlayerState = VictimController ? Cast<AMultiplayerPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState.IsValid() && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.0f); // 점수 더하기
	}
	if (VictimPlayerState.IsValid())
	{
		VictimPlayerState->AddToDefeats(1); // 승리횟수 더하기
	}

	if (IsValid(ElimmedCharacter))
	{
		ElimmedCharacter->Elim(); // 캐릭터 소멸시키기
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
