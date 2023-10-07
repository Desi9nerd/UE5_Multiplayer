#include "MultiplayerGameMode.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

void AMultiplayerGameMode::PlayerEliminated(ABaseCharacter* ElimmedCharacter, AMainPlayerController* VictimController,
	AMainPlayerController* AttackerController)
{
	if (IsValid(ElimmedCharacter))
	{
		ElimmedCharacter->Elim();
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

		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]); // Player ¿ÁΩ√¿€
	}
}
