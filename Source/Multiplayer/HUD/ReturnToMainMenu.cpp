#include "ReturnToMainMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "Multiplayer/Character/BaseCharacter.h"

void UReturnToMainMenu::MenuSetup() // �޴� ���� �� ����
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	TWeakObjectPtr<UWorld> World = GetWorld();
	if (World.IsValid())
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (IsValid(PlayerController))
		{
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			PlayerController->SetInputMode(InputModeData); // �÷��̾� InputMode ����
			PlayerController->SetShowMouseCursor(true); // ���콺 ���O
		}
	}

	if (IsValid(ReturnButton) && false == ReturnButton->OnClicked.IsBound())
	{
		// Dynamic Delegate ���
		ReturnButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}

	TWeakObjectPtr<UGameInstance> GameInstance = GetGameInstance();
	if (GameInstance.IsValid())
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (IsValid(MultiplayerSessionsSubsystem) && 
			false == MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
		{
			// Dynamic Delegate ���
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UReturnToMainMenu::OnDestroySession);
		}
	}
}

bool UReturnToMainMenu::Initialize()
{
	if (false == Super::Initialize())
	{
		return false;
	}

	return true;
}

void UReturnToMainMenu::OnDestroySession(bool bWasSuccessful)
{
	if (false == bWasSuccessful)
	{
		ReturnButton->SetIsEnabled(true);
		return;
	}

	TWeakObjectPtr<UWorld> World = GetWorld();
	if (World.IsValid())
	{
		TWeakObjectPtr<AGameModeBase> GameMode = World->GetAuthGameMode<AGameModeBase>();
		if (GameMode.IsValid())
		{
			GameMode->ReturnToMainMenuHost();
		}
		else
		{
			PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
			if (PlayerController)
			{
				PlayerController->ClientReturnToMainMenuWithTextReason(FText()); // Main Menu�� ���ư�
			}
		}
	}
}

void UReturnToMainMenu::MenuTearDown() // �޴� ����
{
	RemoveFromParent();

	TWeakObjectPtr<UWorld> World = GetWorld();
	if (World.IsValid())
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (IsValid(PlayerController))
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
	if (IsValid(ReturnButton) && ReturnButton->OnClicked.IsBound())
	{
		// Dynamic Delegate ��� ����
		ReturnButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}
	if (IsValid(MultiplayerSessionsSubsystem) && 
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
	{
		// Dynamic Delegate ��� ����
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &UReturnToMainMenu::OnDestroySession);
	}
}

void UReturnToMainMenu::ReturnButtonClicked()
{
	ReturnButton->SetIsEnabled(false);

	TWeakObjectPtr<UWorld> World = GetWorld();
	if (World.IsValid())
	{
		TWeakObjectPtr<APlayerController> FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController.IsValid())
		{
			TWeakObjectPtr<ABaseCharacter> BaseCharacter = Cast<ABaseCharacter>(FirstPlayerController->GetPawn());
			if (BaseCharacter.IsValid())
			{
				BaseCharacter->ServerLeaveGame(); // ���� ����
				BaseCharacter->OnLeftGame.AddDynamic(this, &UReturnToMainMenu::OnPlayerLeftGame); // Dynamic Delegate ���
			}
			else
			{
				ReturnButton->SetIsEnabled(true);
			}
		}
	}
}

void UReturnToMainMenu::OnPlayerLeftGame() // �÷��̾� ���� ����
{
	UE_LOG(LogTemp, Warning, TEXT("OnPlayerLeftGame()"));

	if (IsValid(MultiplayerSessionsSubsystem))
	{
		UE_LOG(LogTemp, Warning, TEXT("MultiplayerSessionsSubsystem valid"));
		MultiplayerSessionsSubsystem->DestroySession(); // ���� ����
	}
}