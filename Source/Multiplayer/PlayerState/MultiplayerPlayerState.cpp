#include "MultiplayerPlayerState.h"
#include "Multiplayer/Character/BaseCharacter.h"
#include "Multiplayer/PlayerController/MainPlayerController.h"
#include "Net/UnrealNetwork.h"

void AMultiplayerPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMultiplayerPlayerState, Defeats);
	DOREPLIFETIME(AMultiplayerPlayerState, Team);
}

void AMultiplayerPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character.IsValid() && Character->Controller)
	{
		Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
		if (Controller.IsValid())
		{
			Controller->SetHUDScore(GetScore()); // Controller를 통해 점수 업데이트
		}
	}
}

void AMultiplayerPlayerState::OnRep_Defeats()
{
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character.IsValid() && Character->Controller)
	{
		Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
		if (Controller.IsValid())
		{
			Controller->SetHUDScore(Defeats); // Controller를 통해 승리횟수 업데이트
		}
	}
}

void AMultiplayerPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount); // 점수 더하기. SetScore함수는 PlayerState.cpp(내장함수)에 정의된 함수

	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character.IsValid() && Character->Controller)
	{
		Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
		if (Controller.IsValid())
		{
			Controller->SetHUDScore(GetScore()); // Controller를 통해 점수 업데이트
		}
	}
}

void AMultiplayerPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount; // 승리횟수 더하기

	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character.IsValid() && Character->Controller)
	{
		Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
		if (Controller.IsValid())
		{
			Controller->SetHUDDefeats(Defeats); // Controller를 통해 승리횟수 업데이트
		}
	}
}

void AMultiplayerPlayerState::OnRep_Team()
{
	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast <ABaseCharacter>(GetPawn());
	if (BaseCharacter.IsValid())
	{
		BaseCharacter->SetTeamColor(Team);
	}
}

void AMultiplayerPlayerState::SetTeam(ETeam TeamToSet)
{
	Team = TeamToSet; // Team 설정

	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast <ABaseCharacter>(GetPawn());
	if (BaseCharacter.IsValid())
	{
		BaseCharacter->SetTeamColor(Team); // Team에 맞는 Team Color 설정
	}
}
