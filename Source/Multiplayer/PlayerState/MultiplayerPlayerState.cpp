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
			Controller->SetHUDScore(GetScore()); // Controller�� ���� ���� ������Ʈ
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
			Controller->SetHUDScore(Defeats); // Controller�� ���� �¸�Ƚ�� ������Ʈ
		}
	}
}

void AMultiplayerPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount); // ���� ���ϱ�. SetScore�Լ��� PlayerState.cpp(�����Լ�)�� ���ǵ� �Լ�

	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character.IsValid() && Character->Controller)
	{
		Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
		if (Controller.IsValid())
		{
			Controller->SetHUDScore(GetScore()); // Controller�� ���� ���� ������Ʈ
		}
	}
}

void AMultiplayerPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount; // �¸�Ƚ�� ���ϱ�

	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character.IsValid() && Character->Controller)
	{
		Controller = Controller == nullptr ? Cast<AMainPlayerController>(Character->Controller) : Controller;
		if (Controller.IsValid())
		{
			Controller->SetHUDDefeats(Defeats); // Controller�� ���� �¸�Ƚ�� ������Ʈ
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
	Team = TeamToSet; // Team ����

	TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast <ABaseCharacter>(GetPawn());
	if (BaseCharacter.IsValid())
	{
		BaseCharacter->SetTeamColor(Team); // Team�� �´� Team Color ����
	}
}
