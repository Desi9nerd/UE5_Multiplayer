#include "CTFGameMode.h"
#include "Multiplayer/Weapon/Flag.h"
#include "Multiplayer/CaptureTheFlag/FlagZone.h"
#include "Multiplayer/GameState/MultiplayerGameState.h"

void ACTFGameMode::PlayerEliminated(ABaseCharacter* ElimmedCharacter, AMainPlayerController* VictimController, AMainPlayerController* AttackerController)
{
	// 부모 클래스(ATeamGameMode)를 부르는 대신 조상클래스(AMultiplayerGameMode)를 부른다.
	AMultiplayerGameMode::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);
}

void ACTFGameMode::FlagCaptured(AFlag* Flag, AFlagZone* Zone)
{
	bool bValidCapture = Flag->GetTeam() != Zone->Team; // 같은 팀인지 체크

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
