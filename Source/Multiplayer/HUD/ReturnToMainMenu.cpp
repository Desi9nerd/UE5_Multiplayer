#include "ReturnToMainMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameModeBase.h"

void UReturnToMainMenu::MenuSetup() // 메뉴 설정 및 띄우기
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
			PlayerController->SetInputMode(InputModeData); // 플레이어 InputMode 설정
			PlayerController->SetShowMouseCursor(true); // 마우스 사용O
		}
	}

	if (IsValid(ReturnButton) && false == ReturnButton->OnClicked.IsBound())
	{
		// Dynamic Delegate 등록
		ReturnButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}

	TWeakObjectPtr<UGameInstance> GameInstance = GetGameInstance();
	if (GameInstance.IsValid())
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (IsValid(MultiplayerSessionsSubsystem) && 
			false == MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
		{
			// Dynamic Delegate 등록
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
				PlayerController->ClientReturnToMainMenuWithTextReason(FText()); // Main Menu로 돌아감
			}
		}
	}
}

void UReturnToMainMenu::MenuTearDown() // 메뉴 해제
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
		// Dynamic Delegate 등록 해제
		ReturnButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}
	if (IsValid(MultiplayerSessionsSubsystem) && 
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
	{
		// Dynamic Delegate 등록 해제
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &UReturnToMainMenu::OnDestroySession);
	}
}

void UReturnToMainMenu::ReturnButtonClicked()
{
	ReturnButton->SetIsEnabled(false);

	if (IsValid(MultiplayerSessionsSubsystem))
	{
		MultiplayerSessionsSubsystem->DestroySession(); // 세션 종료
	}
}