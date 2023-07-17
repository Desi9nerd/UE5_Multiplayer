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
	//meta = (BindWidget)�� ����ϸ� BP�� Button Widget�� �Ʒ��� ������ ����ȴ�. �� �� BP�� C++���� �̸��� ��ġ�Ͽ��� �Ѵ�.
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
