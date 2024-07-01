#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

class UMultiplayerSessionsSubsystem;
class UButton;

/** �޴��� ���ư��� ����
 *
 *	���� ���� ����:
 *	1. UReturnToMainMenu::ReturnButtonClicked()
 *		- 2) BaseCharacter->ServerLeaveGame();
 *		- 6) BaseCharacter->OnLeftGame.AddDynamic(this, &UReturnToMainMenu::OnPlayerLeftGame);
 *	2. ABaseCharacter::ServerLeaveGame()
 *	3. AMultiplayerGameMode::PlayerLeftGame()
 *		- MultiplayerGameState->TopScoringPlayers.Remove, CharacterLeaving->Elim()
 *	4. ABaseCharacter::Elim(), ABaseCharacter::MulticastElim_Implementation()
 *	5. ABaseCharacter::ElimTimerFinished()
 *		- OnLeftGame.Broadcast();
 *	6. UReturnToMainMenu::OnPlayerLeftGame()�� ��������Ʈ Callback��
 *	7. UReturnToMainMenu::OnDestroySession
 *		- PlayerController->ClientReturnToMainMenuWithTextReason(FText()) �ش�
 *
 *	UMultiplayerSessionsSubsystem::DestroySession() >>  SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate
 */

UCLASS()
class MULTIPLAYER_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void MenuSetup(); // �޴� ����
	void MenuTearDown(); // �޴� ����

protected:
	virtual bool Initialize() override;

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnPlayerLeftGame(); // �÷��̾� ���� ����

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
