#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	UGameInstance* GameInstance = GetGameInstance();
	
	if (NumberOfPlayers == 2)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			bUseSeamlessTravel = true;
			World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
		}
	}
}



//
//void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
//{
//	Super::PostLogin(NewPlayer);
//
//	if (GameState)
//	{
//		int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();//게임에 들어온 Player들 숫자
//
//		if (GEngine)
//		{
//			GEngine->AddOnScreenDebugMessage(
//				1,
//				600.f,
//				FColor::Yellow,
//				FString::Printf(TEXT("Players in game: %d"), NumberOfPlayers)
//			);
//
//			APlayerState* PlayerState = NewPlayer->GetPlayerState<APlayerState>();
//			if (PlayerState)
//			{
//				FString PlayerName = PlayerState->GetPlayerName();
//				GEngine->AddOnScreenDebugMessage(
//					2,
//					60.f,
//					FColor::Cyan,
//					FString::Printf(TEXT("%s has joined the game!"), *PlayerName)
//				);
//			}
//		}
//	}
//}
//
//void ALobbyGameMode::Logout(AController* Exiting)
//{
//	Super::Logout(Exiting);
//
//	APlayerState* PlayerState = Exiting->GetPlayerState<APlayerState>();
//	if (PlayerState)
//	{
//		int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
//		GEngine->AddOnScreenDebugMessage(
//			1,
//			600.f,
//			FColor::Yellow,
//			FString::Printf(TEXT("Players in game: %d"), NumberOfPlayers - 1)
//		);
//
//		FString PlayerName = PlayerState->GetPlayerName();
//		GEngine->AddOnScreenDebugMessage(
//			2,
//			60.f,
//			FColor::Cyan,
//			FString::Printf(TEXT("%s has exited the game!"), *PlayerName)
//		);
//	}
//}
//
