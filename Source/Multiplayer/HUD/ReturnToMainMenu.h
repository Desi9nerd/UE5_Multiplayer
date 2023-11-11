#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

/** �޴��� ���ư��� ����
 *
 *	���� ���� ����: UReturnToMainMenu::ReturnButtonClicked() >> UMultiplayerSessionsSubsystem::DestroySession() >>  SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate
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

private:
	UFUNCTION()
	void ReturnButtonClicked();

	UPROPERTY(meta = (BindWidget))
	class UButton* ReturnButton;

	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	UPROPERTY()
	class APlayerController* PlayerController;
};
