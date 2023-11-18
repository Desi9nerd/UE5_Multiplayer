#include "CTFGameMode.h"
#include "Multiplayer/Weapon/Flag.h"
#include "Multiplayer/CaptureTheFlag/FlagZone.h"
#include "Multiplayer/GameState/MultiplayerGameState.h"

void ACTFGameMode::PlayerEliminated(ABaseCharacter* ElimmedCharacter, AMainPlayerController* VictimController, AMainPlayerController* AttackerController)
{
	// �θ� Ŭ����(ATeamGameMode)�� �θ��� ��� ����Ŭ����(AMultiplayerGameMode)�� �θ���.
	AMultiplayerGameMode::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);
}

void ACTFGameMode::FlagCaptured(AFlag* Flag, AFlagZone* Zone)
{
	bool bValidCapture = Flag->GetTeam() != Zone->Team; // ���� ������ üũ

	TWeakObjectPtr<AMultiplayerGameState> MGameState = Cast<AMultiplayerGameState>(GameState);
	if (MGameState.IsValid() && bValidCapture)
	{
		if (Zone->Team == ETeam::ET_BlueTeam)
		{
			MGameState->BlueTeamScores();
		}
		if (Zone->Team == ETeam::ET_RedTeam)
		{
			MGameState->RedTeamScores();
		}
	}
}
