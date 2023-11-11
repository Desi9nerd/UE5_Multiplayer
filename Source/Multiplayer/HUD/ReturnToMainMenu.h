#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

/** 메뉴로 돌아가기 위젯
 *
 *	세션 종료 과정: UReturnToMainMenu::ReturnButtonClicked() >> UMultiplayerSessionsSubsystem::DestroySession() >>  SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate
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
