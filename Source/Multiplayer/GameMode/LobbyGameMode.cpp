#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	TObjectPtr<UGameInstance> GameInstance = GetGameInstance();
	if (IsValid(GameInstance))
	{
		UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		check(Subsystem);

		if (NumberOfPlayers == Subsystem->DesiredNumPublicConnections)
		{
			TObjectPtr<UWorld> World = GetWorld();
			if (IsValid(World))
			{
				bUseSeamlessTravel = true;

				FString MatchType = Subsystem->DesiredMatchType;
				if (MatchType == "FreeForAll")
				{
					World->ServerTravel(FString("/Game/Maps/InGameMap?listen"));
				}
				else if (MatchType == "Teams")
				{
					World->ServerTravel(FString("/Game/Maps/InGameMap_Team?listen"));
				}
				else if (MatchType == "CaptureTheFlag")
				{
					World->ServerTravel(FString("/Game/Maps/InGameMap_CTF?listen"));
				}
			}
		}
	}
}
