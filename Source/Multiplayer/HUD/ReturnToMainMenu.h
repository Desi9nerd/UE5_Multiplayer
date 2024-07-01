#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

class UMultiplayerSessionsSubsystem;
class UButton;

/** 메뉴로 돌아가기 위젯
 *
 *	세션 종료 과정:
 *	1. UReturnToMainMenu::ReturnButtonClicked()
 *		- 2) BaseCharacter->ServerLeaveGame();
 *		- 6) BaseCharacter->OnLeftGame.AddDynamic(this, &UReturnToMainMenu::OnPlayerLeftGame);
 *	2. ABaseCharacter::ServerLeaveGame()
 *	3. AMultiplayerGameMode::PlayerLeftGame()
 *		- MultiplayerGameState->TopScoringPlayers.Remove, CharacterLeaving->Elim()
 *	4. ABaseCharacter::Elim(), ABaseCharacter::MulticastElim_Implementation()
 *	5. ABaseCharacter::ElimTimerFinished()
 *		- OnLeftGame.Broadcast();
 *	6. UReturnToMainMenu::OnPlayerLeftGame()가 델리게이트 Callback됨
 *	7. UReturnToMainMenu::OnDestroySession
 *		- PlayerController->ClientReturnToMainMenuWithTextReason(FText()) 해당
 *
 *	UMultiplayerSessionsSubsystem::DestroySession() >>  SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate
 */

UCLASS()
class MULTIPLAYER_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void MenuSetup(); // 메뉴 설정
	void MenuTearDown(); // 메뉴 해제

protected:
	virtual bool Initialize() override;

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnPlayerLeftGame(); // 플레이어 게임 퇴장

private:
	UFUNCTION()
	void ReturnButtonClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReturnButton;

	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
};
