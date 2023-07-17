#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu.generated.h"

UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup();

protected:

	virtual bool Initialize() override;

private:
	//meta = (BindWidget)을 사용하면 BP의 Button Widget이 아래의 변수와 연결된다. 이 때 BP와 C++변수 이름은 일치하여야 한다.
	UPROPERTY(meta = (BindWidget))
		class UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
		UButton* JoinButton;

	UFUNCTION()
		void HostButtonClicked();

	UFUNCTION()
		void JoinButtonClicked();

	// The subsystem designed to handle all online session functionality
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
};
