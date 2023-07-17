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
		void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")));//접속할 수 있는 Player 숫자를 input으로 만들고 4를 기본값으로 설정한다.

protected:

	virtual bool Initialize() override;
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;//다른 Level로 이동하면 기존의 Level을 지우는 함수.

	//
	// Callbacks for the custom delegates on the MultiplayerSessionsSubsystem
	// MultiplayerSessionsSubsystem.h의 Dynamic Delegate과 연결된다. Dynamic과 연결되는 경우 UFUNCTION()을 꼭 붙여야 한다. 붙이지 않으면 dynamic delegate과 연결되지 않는다.
	UFUNCTION()
		void OnCreateSession(bool bWasSuccessful);

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

	void MenuTearDown();

	// The subsystem designed to handle all online session functionality
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
	
	int32 NumPublicConnections{ 4 };
	FString MatchType{ TEXT("FreeForAll") };
};
